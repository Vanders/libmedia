#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <atheos/soundcard.h>
#include <storage/file.h>

using namespace std;
using namespace os;

struct wave_header
{
	char anID[4];		/* "RIFF" */
	uint32 nSize;		/* Size of the file, minus 8 bytes */
	char anFormat[4];	/* "WAVE" */
};

struct fmt_chunk
{
	char anID[4];			/* "fmt " */
	uint32 nSize;			/* Size of the chunk: we expect 18 for a PCM file (Although some docs say 16..) */
	uint16 nFormat;			/* Data format: 1 = PCM, any other value indicates some form of compression */
	uint16 nChannels;		/* Number of audio channels */
	uint32 nSampleRate;
	uint32 nByteRate;		/* sample rate * channels * bits per sample / 8 */
	uint16 nBlockAlign;		/* channels * bits per sample / 8 */
	uint16 nBitsPerSample;	/* 8 bit, 16 bit etc. */
	uint16 nExtraSize; 		/* Size of any extra data in this chunk; not used for PCM files */
};

struct chunk
{
	char anID[4];			/* Identifier for this chunk I.e. "fmt ", "fact", "data" */
	uint32 nSize;			/* Chunk size */
};

struct fact_chunk
{
	char anID[4];			/* "fact" */
	uint32 nSize;
	uint32 nFileSize;
};

struct data_chunk
{
	char anID[4];			/* "data" */
	uint32 nSize;
};

struct wave_info
{
	uint16 nChannels;
	uint32 nSampleRate;
	uint16 nBitsPerSample;
};

int main( int argc, char *argv[] )
{
	File *pcFile;
	const int nBufferSize = 1024;
	uint8 *pBuffer = new uint8[nBufferSize];
	struct wave_info sInfo;

	try
	{
		pcFile = new File( argv[1] );
	}
	catch( exception &e )
	{
		fprintf( stderr, "Failed to open file \"%s\": %s\n", argv[1], e.what() );
		return 1;
	}
	pcFile->Read( pBuffer, nBufferSize );

	struct wave_header *psHeader = (struct wave_header *)pBuffer;

	printf( "ID: %c%c%c%c\n", psHeader->anID[0], psHeader->anID[1], psHeader->anID[2], psHeader->anID[3] );
	printf( "Size: %d\n", psHeader->nSize );
	printf( "Format: %c%c%c%c\n", psHeader->anFormat[0], psHeader->anFormat[1], psHeader->anFormat[2], psHeader->anFormat[3] );

	if( strncmp( psHeader->anID, "RIFF", 4 ) != 0 || strncmp( psHeader->anFormat, "WAVE", 4 ) != 0 )
	{
		fprintf( stderr, "\"%s\" is not a valid wav file\n", argv[1] );

		delete pBuffer;
		delete pcFile;
		return 1;
	}

#if 0
	uint8 *p = pBuffer + ( sizeof( struct wave_header ) + ( psFmt->nSize + 8 ) );
	for( uint8 n=0; n<8; n++ )
		printf( "%c\n", *p++ );
	printf("\n");
#endif

	uint8 *pNext = pBuffer + sizeof( struct wave_header );
	struct chunk *psChunk = NULL;
	struct fmt_chunk *psFmt = NULL;
	struct fact_chunk *psFact = NULL;
	struct data_chunk *psData = NULL;
	uint8 *pData = NULL;	/* Start of the PCM data */

	while( pNext < ( pBuffer + psHeader->nSize + 8 ) )
	{
		psChunk = (struct chunk *)pNext;

		if( strncmp( psChunk->anID, "fmt ", 4 ) == 0 )
		{
			if( NULL == psFmt )
			{
				psFmt = (struct fmt_chunk *)psChunk;

				printf( "---\n" );
				printf( "Chunk ID: %c%c%c%c\n", psFmt->anID[0], psFmt->anID[1], psFmt->anID[2], psFmt->anID[3] );
				printf( "Size: %d\n", psFmt->nSize );
				printf( "Format: %d\n", psFmt->nFormat );
				printf( "Channels: %d\n", psFmt->nChannels );
				printf( "SampleRate: %d\n", psFmt->nSampleRate );
				printf( "ByteRate: %d\n", psFmt->nByteRate );
				printf( "BlockAlign: %d\n", psFmt->nBlockAlign );
				printf( "BitsPerSample: %d\n", psFmt->nBitsPerSample );
				printf( "ExtraSize: %d\n", psFmt->nExtraSize );
			}
			else
				fprintf( stderr, "Found a second fmt chunk!\n" );
		}
		else if( strncmp( psChunk->anID, "fact", 4 ) == 0 )
		{
			psFact = (struct fact_chunk *)psChunk;

			printf( "---\n" );
			printf( "Chunk ID: %c%c%c%c\n", psFact->anID[0], psFact->anID[1], psFact->anID[2], psFact->anID[3] );
			printf( "Size: %d\n", psFact->nSize );
			printf( "FileSize: %d\n", psFact->nFileSize );
		}
		else if( strncmp( psChunk->anID, "data", 4 ) == 0 )
		{
			psData = (struct data_chunk *)psChunk;

			printf( "---\n" );
			printf( "Chunk ID: %c%c%c%c\n", psData->anID[0], psData->anID[1], psData->anID[2], psData->anID[3] );
			printf( "Size: %d\n", psData->nSize );

			pData = pNext + 8;
			break;
		}
		else
			fprintf( stderr, "Found an unknown chunk \"%c%c%c%c\"!\n", psChunk->anID[0], psChunk->anID[1], psChunk->anID[2], psChunk->anID[3] );

		/* XXXKV: This doesn't take into account any data in the fmt chunk if nExtraSize > 0 */
		pNext += ( psChunk->nSize + 8 );
	}

	/* We have found the fmt & data chunks, and possibly also a fact chunk */
	if( psFmt->nSize != 18 || psFmt->nFormat != 1 )
	{
		fprintf( stderr, "\"%s\" is not a PCM wav file\n", argv[1] );

		delete pBuffer;
		delete pcFile;
		return 1;
	}

	/* Copy the important info */
	sInfo.nChannels = psFmt->nChannels;
	sInfo.nSampleRate = psFmt->nSampleRate;
	sInfo.nBitsPerSample = psFmt->nBitsPerSample;

	printf( "\npData is at 0x%p (Offset %d)\n", pData, (int)(pData - pBuffer) );

	int nDataSize = nBufferSize - (int)(pData - pBuffer);
	printf( "nDataSize = %d\n", nDataSize );

	int fd = open( "/dev/sound/i810/dsp/0", O_RDONLY );
	if( fd >= 0 )
	{
		ioctl( fd, SNDCTL_DSP_CHANNELS, sInfo.nChannels );
		ioctl( fd, SNDCTL_DSP_SPEED, sInfo.nSampleRate );
		int nFmt = AFMT_QUERY;	/* Safe default */
		if( sInfo.nBitsPerSample == 8 )
			nFmt = AFMT_U8;
		else if( sInfo.nBitsPerSample == 16 )
			nFmt = AFMT_S16_LE;	/* "RIFX" files are Big Endian, 16bit samples are always signed */
		else
		{
			fprintf( stderr, "Invalid Bits Per Sample %d\n", sInfo.nBitsPerSample );
		}
		ioctl( fd, SNDCTL_DSP_SETFMT, nFmt );

		while( nDataSize > 0 )
		{
			write( fd, pData, nDataSize );

			nDataSize = pcFile->Read( pBuffer, nBufferSize );
			pData = pBuffer;
		}
		close( fd );
	}

	delete pBuffer;
	delete pcFile;
	return 0;
}

