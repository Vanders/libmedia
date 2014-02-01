#include <atheos/kdebug.h>

#include <pipeline.h>
#include <stage.h>
#include <buffer.h>
#include <packet.h>

using namespace os;
using namespace media;

StageNode::StageNode( Stage *pcStage, int nBuffers )
{
	m_pcStage = pcStage;
	m_nBuffers = nBuffers;
	m_cIdentifier = "Unknown";

	/* An array of Buffer pointers */
	m_vpcBuffers = new Buffer*[m_nBuffers];
}

StageNode::~StageNode()
{
	/* Delete all of our associated Buffers */
	for( int i = 0; i < m_nBuffers; i++ )
		delete m_vpcBuffers[i];
	delete m_vpcBuffers;

	/* Delete the associated Stage */
	delete m_pcStage;
}

/* Associate a Buffer with the output nOutput.  We own the Buffer and it will be deleted by us */
status_t StageNode::AddBuffer( Buffer *pcBuffer, int nOutput )
{
	if( nOutput < 0 || nOutput > m_nBuffers )
		return EINVAL;

	m_vpcBuffers[nOutput] = pcBuffer;
	return EOK;
}

Buffer * StageNode::GetBuffer( int nOutput )
{
	if( nOutput < 0 || nOutput > m_nBuffers )
		return NULL;
	return m_vpcBuffers[nOutput];
}

Pipeline::Pipeline( String cIdentifier )
{
	m_cIdentifier = cIdentifier;
}

Pipeline::~Pipeline()
{
}

Packet * Pipeline::AllocPacket( void )
{
	Packet *pcPacket;

	try
	{
		pcPacket = new Packet();
	}
	catch( std::exception &e )
	{
		dbprintf( "%s: %s", __FUNCTION__, e.what() );
		throw e;
	}

	return pcPacket;
}

status_t Pipeline::FreePacket( Packet *pcPacket )
{
	if( NULL == pcPacket )
		return EINVAL;
	delete pcPacket;

	return EOK;
}

InputPipeline::InputPipeline( String cIdentifier ) : Pipeline( cIdentifier )
{
}

InputPipeline::~InputPipeline()
{
	/* Delete all of the StageNodes.  The StageNodes own their associated Buffers and will delete them for us */
	std::list<StageNode *>::iterator i;
	for( i = m_vpcStages.begin(); i != m_vpcStages.end(); i++ )
		delete (*i);
	m_vpcStages.clear();
}

/*
   Add a stage to this pipeline.  Stages are added from the top, down E.g. SOURCE first, then down
   the pipeline to DECODE (or EFFECT)  When a stage is added a Buffer is created for each output.

   When a Stage is added, a StageNode is created.  This is used to keep the Stage & it's associated
   downstream Buffers in one logical place.  Each Buffer that is created is passed to the StageNode.
   The StageNode owns the Stage & all of the Buffers added to it.

   Connect() is used to join stages together.
*/

#include <iostream>
using namespace std;

status_t InputPipeline::AddStage( InputStage *pcStage, String &cIdentifier )
{
	if( NULL == pcStage )
		return EINVAL;

	pcStage->SetPipeline( this );

	int nOutputCount = pcStage->GetOutputCount();
	StageNode *pcNode = new StageNode( pcStage, nOutputCount );

	std::cerr << "stage has " << nOutputCount << " outputs" << std::endl;

	/* Create a Buffer for each output */
	if( nOutputCount > 0 )
	{
		Buffer *pcBuffer;
		int nOutput;

		for( nOutput = 0; nOutput < nOutputCount; nOutput++ )
		{
			/* Create a new Buffer and associate it with this Stage */
			pcBuffer = new Buffer( pcStage, nOutput );
			pcNode->AddBuffer( pcBuffer, nOutput );

			/* If this is a SOURCE plugin, start the buffer now */
			if( pcStage->GetInputInterface() == SOURCE )
			{
				std::cerr << "this stage is SOURCE: starting buffer" << std::endl;
				pcBuffer->Start();
			}
		}
	}

	/* Create a unique identifier for this stage */
	String cName;
	int nCount = 0;

	cName = pcStage->GetName();

	std::list<StageNode *>::iterator i;
	for( i = m_vpcStages.begin(); i != m_vpcStages.end(); i++ )
		if( cName == (*i)->GetName() )
			nCount++;

	cIdentifier.Format( "%s-%d", cName.c_str(), nCount );
	pcNode->SetIdentifier( cIdentifier );

	std::cerr << "this stage is identified as \"" << cIdentifier.const_str() << "\"" << std::endl;

	/* The stage has been added to the pipeline.  We now own it. */
	m_vpcStages.push_back( pcNode );

	return EOK;
}

/*
   Return the Buffer associated with the stage.  This allows the caller to retrieve the data from a buffer
   that is at the end of the pipeline.
*/
Buffer * InputPipeline::GetBuffer( String cStage, int nOutput )
{
	StageNode *pcStageNode = NULL;

	/* Find the stage */
	std::list<StageNode *>::iterator i;
	for( i = m_vpcStages.begin(); i != m_vpcStages.end(); i++ )
	{
		if( (*i)->GetIdentifier() == cStage )
			pcStageNode = (*i);
	}

	if( NULL == pcStageNode )
		return NULL;

	/* Get the buffer */
	Buffer *pcBuffer = pcStageNode->GetBuffer( nOutput );

	return pcBuffer;
}

/*
	Connect the input of "downstream" to the given output of "upstream"

	XXXKV: Perhaps we should check that the interfaces match?
*/
status_t InputPipeline::Connect( String cDownstream, String cUpstream, int nOutput )
{
	status_t nError;
	StageNode *pcStageNode1 = NULL, *pcStageNode2 = NULL;

	/* Find both stages */
	std::list<StageNode *>::iterator i;
	for( i = m_vpcStages.begin(); i != m_vpcStages.end(); i++ )
	{
		if( (*i)->GetIdentifier() == cDownstream )
			pcStageNode1 = (*i);
		if( (*i)->GetIdentifier() == cUpstream )
			pcStageNode2 = (*i);
	}

	if( NULL == pcStageNode1 || NULL == pcStageNode2 )
		return ENOENT;

	/* Get the buffer from stage2 */
	Buffer *pcBuffer = pcStageNode2->GetBuffer( nOutput );
	if( NULL == pcBuffer )
		return EINVAL;

	/* Connect the stage1 input to the buffer */
	InputStage *pcStage = static_cast<InputStage *>( pcStageNode1->GetStage() );
	nError = pcStage->Connect( pcBuffer );
	if( nError != EOK )
		return nError;

	/* Start the buffers for stage1 */
	int nBuffers = pcStageNode1->GetBufferCount();
	for( int n = 0; n < nBuffers; n++ )
	{
		Buffer *pcBuffer = pcStageNode1->GetBuffer( n );
		if( pcBuffer )
		{
			status_t nError = pcBuffer->Start();
			if( nError != EOK )
				return nError;
		}
	}

	return EOK;
}

status_t InputPipeline::Start( void )
{
	std::list<StageNode *>::iterator i;
	for( i = m_vpcStages.begin(); i != m_vpcStages.end(); i++ )
	{
		int nBuffers = (*i)->GetBufferCount();
		for( int n = 0; n < nBuffers; n++ )
		{
			Buffer *pcBuffer = (*i)->GetBuffer( n );
			if( pcBuffer )
			{
				status_t nError = pcBuffer->Start();
				if( nError != EOK )
					return nError;
			}
		}
	}

	return EOK;
}

status_t InputPipeline::Stop( void )
{
	std::list<StageNode *>::iterator i;
	for( i = m_vpcStages.begin(); i != m_vpcStages.end(); i++ )
	{
		int nBuffers = (*i)->GetBufferCount();
		for( int n = 0; n < nBuffers; n++ )
		{
			Buffer *pcBuffer = (*i)->GetBuffer( n );
			if( pcBuffer )
			{
				status_t nError = pcBuffer->Stop();
				if( nError != EOK )
					return nError;
			}
		}
	}

	return EOK;
}


