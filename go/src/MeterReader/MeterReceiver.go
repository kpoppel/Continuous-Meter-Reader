package MeterReader

import (
	"encoding/binary"
	"github.com/golang/protobuf/proto"
	"hash/crc32"
	"io"
	"log"
	"os"
)

const MAX_PROTOBUF_MSG_LEN = 28

var castagnoliTable = crc32.MakeTable(crc32.IEEE) // see http://golang.org/pkg/hash/crc32/#pkg-constants

func HandleProtoClient(conn io.ReadCloser, c chan *CounterUpdate) {
	var len uint32

	log.Println("Connection established")
	//Close the connection when the function exits
	defer conn.Close()

	for {

		// Find the first occurrence of the magic string which is AA
		buf := make([]byte, 1)
		seen := 0
		garbage := 0
		for seen != 2 {
			_, err := conn.Read(buf)
			if err == io.EOF {
				log.Printf("EOF reached")
				break
			}

			CheckError(err)

			if buf[0] == 'A' {
				seen += 1
			} else {
				seen = 0
				garbage += 1
			}

		}
		if garbage > 0 {
			log.Printf("Discarded %d bytes of garbage\n", garbage)
		}

		//Read the length field
		err := binary.Read(conn, binary.BigEndian, &len)
		if err == io.EOF {
			log.Printf("EOF reached")
			break
		}

		CheckError(err)
		log.Println("len=", len)

		if len > MAX_PROTOBUF_MSG_LEN {
			log.Println("Message length unrealistically large. Skipping. len=", len)
			continue
		}

		//Create a data buffer of type byte slice with capacity for the message
		data := make([]byte, len)
		//Read the data waiting on the connection and put it in the data buffer
		n, err := conn.Read(data)
		if err == io.EOF {
			log.Printf("EOF reached")
			break
		}
		CheckError(err)

		// Read the checksum and match it against the received data
		crc := crc32.New(castagnoliTable)
		crc.Write(data)
		var expectedCRC uint32
		err = binary.Read(conn, binary.BigEndian, &expectedCRC)
		if err == io.EOF {
			log.Printf("EOF reached")
			break
		}
		CheckError(err)

		if crc.Sum32() != expectedCRC {
			log.Println("Checksum mismatch, skipping")
			continue
		}

		protodata := new(CounterUpdate)
		//Convert all the data retrieved into the CounterUpdate struct type
		err = proto.Unmarshal(data[0:n], protodata)
		CheckError(err)
		//Push the protobuf message into a channel
		c <- protodata
	}
}

func CheckError(err error) {
	if err != nil {
		log.Fatal("Fatal error: %s", err.Error())
		os.Exit(1)
	}
}