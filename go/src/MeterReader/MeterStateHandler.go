package MeterReader

import (
	"log"
	"time"
)

type Meter struct {
	MeterId       uint32
	Name          string
	Unit          string
	CurrentSeries uint32
	StartCount    uint64
	LastCount     uint64
}

type MeterUpdate struct {
	MeterId    uint32    `json:"meter"`
	MeasuredAt time.Time `json:"measured_at"`
	Unit       string    `json:"unit"`
	Name       string    `json:"name"`
	Value      uint64    `json:"value"`
}

type MeterStateHandler struct {
	mdb    MeasurementStorer
	meters map[uint32]*Meter
}

func NewMeterStateHandler() *MeterStateHandler {
	msh := new(MeterStateHandler)
	mdb := NewMeterDB()
	msh.mdb = mdb
	msh.meters = mdb.GetMeterState()

	return msh
}

func (msh *MeterStateHandler) Handle(queue chan *CounterUpdate) chan *MeterUpdate {
	outch := make(chan *MeterUpdate)

	go func() {
		for msg := range queue {
			tmsg := msh.Translate(msg)
			if tmsg != nil {
				outch <- tmsg
			}
		}
	}()

	return outch
}

func (msh *MeterStateHandler) Translate(msg *CounterUpdate) *MeterUpdate {

	//Retreive client information from the protobuf message
	MeterId := msg.GetMeterId()

	meter, ok := msh.meters[MeterId]
	if !ok {
		log.Printf("Ignoring unknown new meter instance id %d\n", MeterId)
		return nil
	}

	SeriesId := msg.GetSeriesId()
	CurrentCounterValue := msg.GetCurrentCounterValue()

	if meter.CurrentSeries != SeriesId {
		meter.CurrentSeries = SeriesId
		meter.StartCount = meter.LastCount

		msh.mdb.UpdateMeterState(meter)
	}

	newCount := meter.StartCount + CurrentCounterValue

	changed := false
	if newCount != meter.LastCount {
		changed = true
	}
	meter.LastCount = newCount

	log.Printf("meterid=%d series=%d counter=%d -> absolute=%d\n", MeterId, SeriesId, CurrentCounterValue, meter.LastCount)

	umsg := MeterUpdate{
		MeterId:    meter.MeterId,
		MeasuredAt: time.Now(),
		Unit:       meter.Unit,
		Name:       meter.Name,
		Value:      meter.LastCount,
	}

	if changed {
		msh.mdb.InsertMeasurement(&umsg)
		return &umsg
	}

	return nil
}
