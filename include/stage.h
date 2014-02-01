#ifndef __F_MEDIA_STAGE_H_
#define __F_MEDIA_STAGE_H_

#include <list>

#include <interface.h>

namespace media
{

class Pipeline;
class Packet;
class Buffer;

class Stage
{
	public:
		Stage();
		virtual ~Stage();

		/* Each Stage must have a unique name */
		virtual os::String GetName( void );

		/* What is the output interfaces for this stage? */
		virtual interface_t GetOutputInterface( void )
		{
			return NONE;
		};

		virtual interface_t GetInputInterface( void )
		{
			return NONE;
		};

		/* Does the stage recognise the packet? */
		virtual bool Check( Packet *pcPacket )
		{
			return false;
		};

		/* Return a packet from the output stream nInterface */
		virtual status_t GetPacket( Packet **ppcPacket, int nInterface );

		virtual void SetPipeline( Pipeline *pcPipeline )
		{
			m_pcPipeline = pcPipeline;
		};

	protected:
		Pipeline *m_pcPipeline;
};

/* An InputStage takes an input stream from an upstream Buffer and produces one or more output streams.
   All of the output streams must have the same interface.  An InputStage may only be connected to one
   upstream Buffer. */

class InputStage : public Stage
{
	public:
		InputStage();
		virtual ~InputStage();

		/* How many output streams are produced? */
		virtual int GetOutputCount( void );

		/* Connect this stage to an upstream Buffer.  */
		virtual status_t Connect( Buffer *pcBuffer );
};

class SourceStage : public InputStage, public SourceInterface
{
	public:
		SourceStage(){};
		virtual ~SourceStage(){};
};

class DemuxStage : public InputStage, public DemuxInterface
{
	public:
		DemuxStage(){};
		virtual ~DemuxStage(){};
};

class DecodeStage : public InputStage, public DecodeInterface
{
	public:
		DecodeStage(){};
		virtual ~DecodeStage(){};
};

}

#endif	/* __F_MEDIA_STAGE_H_ */
