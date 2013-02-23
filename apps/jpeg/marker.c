#include "datatype.h"
#include "config.h"
#include "markdata.h"

// Header for JPEG Encoder

void writeMarkers(
	UINT8 *outoutBuffer, UINT32 imageFormat, UINT32 width, UINT32 height
) {
	UINT16 i, headerLength;
	UINT8 numberOfComponents;

	// Start of image marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xD8;

	// Quantization table marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xDB;

	// Quantization table length
	*outoutBuffer++ = 0x00;
	*outoutBuffer++ = 0x84;

	// Pq, Tq
	*outoutBuffer++ = 0x00;

	// Lqt table
	for (i = 0; i < 64; i++)
		*outoutBuffer++ = Lqt[i];

	// Pq, Tq
	*outoutBuffer++ = 0x01;

	// Cqt table
	for (i = 0; i < 64; i++)
		*outoutBuffer++ = Cqt[i];

	// huffman table(DHT)
	for (i = 0; i < 210; i++) {
		*outoutBuffer++ = (UINT8) (markerdata[i] >> 8);
		*outoutBuffer++ = (UINT8) markerdata[i];
	}

	if (imageFormat == GRAY)
		numberOfComponents = 1;
	else
		numberOfComponents = 3;

	// Frame header(SOF)

	// Start of frame marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xC0;

	headerLength = (UINT16) (8 + 3 * numberOfComponents);

	// Frame header length	
	*outoutBuffer++ = (UINT8) (headerLength >> 8);
	*outoutBuffer++ = (UINT8) headerLength;

	// Precision (P)
	*outoutBuffer++ = 0x08;

	// image height
	*outoutBuffer++ = (UINT8) (height >> 8);
	*outoutBuffer++ = (UINT8) height;

	// image width
	*outoutBuffer++ = (UINT8) (width >> 8);
	*outoutBuffer++ = (UINT8) width;

	// Nf
	*outoutBuffer++ = numberOfComponents;

	if (imageFormat == GRAY) {
		*outoutBuffer++ = 0x01;
		*outoutBuffer++ = 0x11;
		*outoutBuffer++ = 0x00;
	} else {
		*outoutBuffer++ = 0x01;

		*outoutBuffer++ = 0x21;

		*outoutBuffer++ = 0x00;

		*outoutBuffer++ = 0x02;
		*outoutBuffer++ = 0x11;
		*outoutBuffer++ = 0x01;

		*outoutBuffer++ = 0x03;
		*outoutBuffer++ = 0x11;
		*outoutBuffer++ = 0x01;
	}

	// Scan header(SOF)

	// Start of scan marker
	*outoutBuffer++ = 0xFF;
	*outoutBuffer++ = 0xDA;

	headerLength = (UINT16) (6 + (numberOfComponents << 1));

	// Scan header length
	*outoutBuffer++ = (UINT8) (headerLength >> 8);
	*outoutBuffer++ = (UINT8) headerLength;

	// Ns
	*outoutBuffer++ = numberOfComponents;

	if (imageFormat == GRAY) {
		*outoutBuffer++ = 0x01;
		*outoutBuffer++ = 0x00;
	} else {
		*outoutBuffer++ = 0x01;
		*outoutBuffer++ = 0x00;

		*outoutBuffer++ = 0x02;
		*outoutBuffer++ = 0x11;

		*outoutBuffer++ = 0x03;
		*outoutBuffer++ = 0x11;
	}

	*outoutBuffer++ = 0x00;
	*outoutBuffer++ = 0x3F;
	*outoutBuffer++ = 0x00;
}
