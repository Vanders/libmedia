#include <pipeline.h>
#include <stage.h>
#include <interface.h>
#include <packet.h>
#include <buffer.h>

using namespace os;
using namespace media;

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

class WaveStage : public DemuxStage
{
	public:
		WaveStage();
		~WaveStage();

		String GetName( void ){ return "demux/wave"; };

		interface_t GetInputInterface( void ){ return DEMUX; };
		interface_t GetOutputInterface( void ){ return OUTPUT; };

		bool Check( Packet *pcPacket );

		void GetInputMimeType( String &cFormat )
		{
			cFormat = "audio/wav";
		}

		/* We can only provide a single stream of data */
		int GetOutputCount( void ){ return 1; };
		status_t GetPacket( Packet **ppcPacket, int nInterface );

		status_t Connect( Buffer *pcBuffer );

	private:
		Buffer *m_pcUpstream;
		uint64 m_nPacketCount;	/* How many packets have we processed? */

		uint32 m_nDataOffset;	/* Offset to the start of the audio data after the chunks */

		uint16 m_nChannels;
		uint32 m_nSampleRate;
		uint16 m_nBitsPerSample;
};

WaveStage::WaveStage()
{
	m_pcUpstream = NULL;
	m_nPacketCount = 0;

	m_nDataOffset = 0;

	m_nChannels = 0;
	m_nSampleRate = 0;
	m_nBitsPerSample = 0;
}

WaveStage::~WaveStage()
{
}

#include <iostream>
using namespace std;

bool WaveStage::Check( Packet *pcPacket )
{
	if( NULL == pcPacket )
	{
		cerr << "pcPacket is NULL" << endl;
		return false;
	}

	/* Is this a RIFF WAVE file? */
	const uint8 *pData = pcPacket->GetData();

	struct wave_header *psHeader = (struct wave_header *)pData;
	if( strncmp( psHeader->anID, "RIFF", 4 ) != 0 || strncmp( psHeader->anFormat, "WAVE", 4 ) != 0 )
		return false;

	/* Find the chunks and check the format etc. is valid */
	const uint8 *pNext = pData + sizeof( struct wave_header );
	struct chunk *psChunk = NULL;
	struct fmt_chunk *psFmt = NULL;
	struct fact_chunk *psFact = NULL;
	struct data_chunk *psData = NULL;
	uint16 nExtraSize = 0;

	while( pNext < ( pData + psHeader->nSize + 8 ) )
	{
		psChunk = (struct chunk *)pNext;
		nExtraSize = 0;

		if( strncmp( psChunk->anID, "fmt ", 4 ) == 0 )
		{
			if( NULL == psFmt )
			{
				psFmt = (struct fmt_chunk *)psChunk;
				nExtraSize = psFmt->nExtraSize;
			}
			else
				dbprintf( "found a second fmt chunk\n" );
		}
		else if( strncmp( psChunk->anID, "fact", 4 ) == 0 )
		{
			psFact = (struct fact_chunk *)psChunk;
		}
		else if( strncmp( psChunk->anID, "data", 4 ) == 0 )
		{
			psData = (struct data_chunk *)psChunk;

			m_nDataOffset = (pNext + 8) - pData;
			break;
		}
		else
			dbprintf( "found an unknown chunk \"%c%c%c%c\"!\n", psChunk->anID[0], psChunk->anID[1], psChunk->anID[2], psChunk->anID[3] );

		pNext += ( psChunk->nSize + 8 + nExtraSize );
	}

	if( NULL == psData || NULL == psFmt )
		return false;

	if( psFmt->nSize != 18 || psFmt->nFormat != 1 )
		return false;

	/* Copy the important info */
	m_nChannels = psFmt->nChannels;
	m_nSampleRate = psFmt->nSampleRate;
	m_nBitsPerSample = psFmt->nBitsPerSample;

	/* This would appear to be a RIFF WAVE file */
	return true;
}

status_t WaveStage::GetPacket( Packet **ppcPacket, int nInterface )
{
	if( nInterface > 0 || NULL == m_pcUpstream )
	{
		cerr << "GetPacket() early failure" << endl;
		return EINVAL;
	}

	Packet *pcPacket;

	if( m_nPacketCount == 0 )
	{
		/* We have to handle the first packet a little differently and skip the header & chunk data */
		Packet *pcInPacket;

		pcInPacket = m_pcUpstream->GetPacket( false );
		if( NULL == pcInPacket )
		{
			//cerr << "failed to get upstream packet" << endl;
			return EIO;
		}

		pcPacket = m_pcPipeline->AllocPacket();
		if( NULL == pcPacket )
		{
			cerr << "failed to allocate packet" << endl;
			return ENOMEM;
		}

		pcPacket->SetData( ( pcInPacket->GetData() + m_nDataOffset ), ( pcInPacket->GetDataSize() - m_nDataOffset ) );

		m_pcPipeline->FreePacket( pcInPacket );
	}
	else
	{
		pcPacket = m_pcUpstream->GetPacket();
		if( NULL == pcPacket )
		{
			//cerr << "failed to get upstream packet" << endl;
			return EIO;
		}
	}

	AudioPacketInfo *pcInfo = new AudioPacketInfo();
	pcInfo->nChannels = m_nChannels;
	pcInfo->nSampleRate = m_nSampleRate;
	pcInfo->nBitsPerSample = m_nBitsPerSample;
	if( m_nBitsPerSample == 8 )
		pcInfo->eFormat = PCM_UNSIGNED_8;
	else
		pcInfo->eFormat = PCM_SIGNED_LE;	/* "RIFX" files are Big Endian, 16bit samples are always signed */

	pcPacket->SetInfo( pcInfo );

	*ppcPacket = pcPacket;
	m_nPacketCount++;
	return EOK;
}

status_t WaveStage::Connect( Buffer *pcBuffer )
{
	m_pcUpstream = pcBuffer;
	return EOK;
}

extern "C"
{
	Stage * GetInstance( void )
	{
		return new WaveStage();
	}

};

