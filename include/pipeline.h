#ifndef __F_MEDIA_PIPELINE_H_
#define __F_MEDIA_PIPELINE_H_

#include <stage.h>

#include <atheos/areas.h>
#include <atheos/types.h>
#include <util/string.h>

#include <list>

namespace media
{

class Packet;
class Buffer;

/* We need to keep track of each Stage and it's associated Buffers. */
class StageNode
{
	public:
		StageNode( Stage *pcStage, int nBuffers );
		~StageNode();

		/* Set & get the unique identifier from the Stage */
		void SetIdentifier( os::String cIdentifier )
		{
			m_cIdentifier = cIdentifier;
		};
		os::String GetIdentifier( void )
		{
			return m_cIdentifier;
		};
		/* Return the name of the stage associated with this node */
		os::String GetName( void )
		{
			return m_pcStage->GetName();
		};
		Stage * GetStage( void )
		{
			return m_pcStage;
		};

		/* Associate a Buffer with the output interface nOutput */
		status_t AddBuffer( Buffer *pcBuffer, int nOutput );
		/* Number of buffers associated with this stage */
		int GetBufferCount( void )
		{
			return m_nBuffers;
		};
		/* Return the Buffer associated with the output interface nOutput */
		Buffer * GetBuffer( int nOutput );

	private:
		Stage *m_pcStage;
		int m_nBuffers;
		os::String m_cIdentifier;

		/* An array of Buffers */
		Buffer **m_vpcBuffers;
};

class Pipeline
{
	public:
		Pipeline( os::String cIdentifier );
		virtual ~Pipeline();

		virtual Packet* AllocPacket( void );
		virtual status_t FreePacket( Packet *pcPacket );

		virtual os::String GetIdentifer( void ){ return m_cIdentifier; };

		/* Start & Stop all of the buffers in the pipeline */
		virtual status_t Start( void ){ return ENOSYS; };
		virtual status_t Stop( void ){ return ENOSYS; };

	protected:
		os::String m_cIdentifier;
};

class InputPipeline : public Pipeline
{
	public:
		InputPipeline( os::String cIdentifier );
		~InputPipeline();

		status_t AddStage( InputStage *pcStage, os::String &cIdentifier );

		/* Return the buffer associated with the numbered output if the stage */
		Buffer * GetBuffer( os::String cStage, int nOutput );

		/* Connect the "downstream" input to the numbered output of "upstream" */
		status_t Connect( os::String cDownstream, os::String cUpstream, int nOutput );

		status_t Start( void );
		status_t Stop( void );
	private:
		std::list <StageNode *> m_vpcStages;
};

}

#endif	/* __F_MEDIA_PIPELINE_H_ */

