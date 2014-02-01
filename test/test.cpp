#include <pipeline.h>
#include <stage.h>
#include <packet.h>
#include <buffer.h>

#include <atheos/image.h>
#include <storage/file.h>

#include <iostream>

using namespace std;
using namespace os;
using namespace media;

int main( void )
{
	Stage* (*pGetInstance)(void) = NULL;

	/* Load the file plugin */
	image_id hSourceFile = load_library( "../plugins/file", 0 );
	if( hSourceFile < 0 )
	{
		cerr << "failed to load file plugin" << endl;
		return 1;
	}

	if( get_symbol_address( hSourceFile, "GetInstance", -1, (void**)&pGetInstance ) < 0 )
	{
		cerr << "failed to find GetInstance()" << endl;
		unload_library( hSourceFile );
		return 1;
	}

	SourceStage *pcSource = static_cast<SourceStage *>( pGetInstance() );
	printf( "pcSource at 0x%p is \"%s\"\n", pcSource, pcSource->GetName().c_str() );

	/* Load the wave plugin */
	image_id hDemuxWave = load_library( "../plugins/wave", 0 );
	if( hDemuxWave < 0 )
	{
		cerr << "failed to load wave plugin" << endl;
		unload_library( hSourceFile );
		return 1;
	}

	if( get_symbol_address( hDemuxWave, "GetInstance", -1, (void**)&pGetInstance ) < 0 )
	{
		cerr << "failed to find GetInstance()" << endl;
		unload_library( hSourceFile );
		unload_library( hDemuxWave );
		return 1;
	}

	DemuxStage *pcDemux = static_cast<DemuxStage *>( pGetInstance() );
	printf( "pcDemux at 0x%p is \"%s\"\n", pcDemux, pcDemux->GetName().c_str() );

	String cSourceIdentifier, cDemuxIdentifier;
	InputPipeline *pcPipeline;
	Buffer *pcSourceBuffer, *pcOutputBuffer;
	bool bRun = true;

	string cInfile, cOutfile;
	cout << "Input file? ";
	cin >> cInfile;
	cout << "Output file? ";
	cin >> cOutfile;

	try
	{
		pcSource->OpenUri( cInfile );
	}
	catch( exception &e )
	{
		cerr << "cant open \"" << cInfile << "\" for reading" << endl;
		goto out;
	}

	File *pcSink;
	try
	{
		pcSink = new File( cOutfile, O_CREAT|O_WRONLY );
	}
	catch( exception &e )
	{
		cerr << "cant open \"" << cOutfile << "\" for writing" << endl;
		goto out;
	}

	/* Create a Pipeline and add the stages to it */
	pcPipeline = new InputPipeline( "input_test" );

	pcPipeline->AddStage( pcSource, cSourceIdentifier );
	cout << "Source added as \"" << cSourceIdentifier.const_str() << "\"" << endl;
	pcPipeline->AddStage( pcDemux, cDemuxIdentifier );
	cout << "Demux added as \"" << cDemuxIdentifier.const_str() << "\"" << endl;

	/* Get the source buffer */
	pcSourceBuffer = pcPipeline->GetBuffer( cSourceIdentifier, 0 );
	if( NULL == pcSourceBuffer )
	{
		cerr << "failed to get buffer for " << cSourceIdentifier.const_str() << endl;
		goto out;
	}

	/* Check that the Demux plugin can handle the input */
	Packet *pcPacket = pcSourceBuffer->GetPacket( false, false );
	if( pcDemux->Check( pcPacket ) == false )
	{
		cerr << "\"" << cInfile << "\" is not a RIFF WAVE file" << endl;
		goto out;
	}

	/* Connect the file source to the wave demux */
	pcPipeline->Connect( cDemuxIdentifier, cSourceIdentifier, 0 );

	/* Get the output buffer */
	pcOutputBuffer = pcPipeline->GetBuffer( cDemuxIdentifier, 0 );
	if( NULL == pcSourceBuffer )
	{
		cerr << "failed to get buffer for " << cDemuxIdentifier.const_str() << endl;
		goto out;
	}
	pcOutputBuffer->SetMinMax( 10, 20 );

	/* Start playing */
	do
	{
		int n=0;
		while( true )
		{
			pcPacket = pcOutputBuffer->GetPacket( false );
			if( NULL == pcPacket )
			{
				//cerr << "failed to read packet" << endl;
				bRun = false;
				break;
			}

			size_t nSize = pcPacket->GetDataSize();
			//cout << "read packet " << n++ << " (" << nSize << ")" << endl;

			pcSink->Write( pcPacket->GetData(), nSize );

			/* It's out packet now; better delete it */
			delete pcPacket;

			/* Display some info */
			fprintf( stdout, "\rSource Buffer Fill\t%d\tOutput Buffer Fill\t%d    ", pcSourceBuffer->GetCount(), pcOutputBuffer->GetCount() );
			fflush( stdout );
		}
	}
	while( bRun );

	pcSink->Flush();
	delete pcSink;

	fprintf( stdout, "\n" );

	/* Stop the pipeline */
	pcPipeline->Stop();

out:
	/* Clean up & exit */
	delete pcPipeline;
	unload_library( hSourceFile );
	unload_library( hDemuxWave );

	return 0;
}

