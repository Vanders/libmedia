#include <pipeline.h>
#include <stage.h>
#include <interface.h>
#include <packet.h>

#include <storage/file.h>

using namespace os;
using namespace media;

class FileStage : public SourceStage
{
	public:
		FileStage();
		~FileStage();

		String GetName( void ){ return "source/file"; };

		interface_t GetInputInterface( void ){ return SOURCE; };
		interface_t GetOutputInterface( void ){ return DEMUX; };

		status_t OpenUri( String cUri );

		/* We can only provide a single stream of data */
		int GetOutputCount( void ){ return 1; };
		status_t GetPacket( Packet **ppcPacket, int nInterface );

		/* Can't connect us to anything upstream as we are SOURCE */
		status_t Connect( Buffer *pcBuffer ){ return EINVAL; };

	private:
		File *m_pcFile;
		uint8 *m_pcData;
};

FileStage::FileStage()
{
	m_pcFile = NULL;
	m_pcData = new uint8[4096];
}

FileStage::~FileStage()
{
	if( m_pcFile )
		delete m_pcFile;
	delete m_pcData;
}

#include <iostream>

status_t FileStage::OpenUri( String cUri )
{
	if( m_pcFile )
		return EINVAL;

	try
	{
		m_pcFile = new File( cUri );
	}
	catch( std::exception &e )
	{
		dbprintf( "%s: %s\n", __FUNCTION__, e.what() );
		throw( e );
	}

	std::cerr << "opened \"" << cUri.const_str() << "\" for reading" << std::endl;

	return EOK;
}

status_t FileStage::GetPacket( Packet **ppcPacket, int nInterface )
{
	if( nInterface > 0 || NULL == m_pcFile || NULL == m_pcPipeline )
		return EINVAL;

	Packet *pcPacket = m_pcPipeline->AllocPacket();
	if( NULL == pcPacket )
		return ENOMEM;

	size_t nSize = m_pcFile->Read( m_pcData, 4096 );
	if( nSize <= 0 )
		return EIO;

	pcPacket->SetData( m_pcData, nSize );
	*ppcPacket = pcPacket;

	return EOK;
}

extern "C"
{
	Stage * GetInstance( void )
	{
		return new FileStage();
	}

};

