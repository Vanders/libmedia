#ifndef __F_MEDIA_INTERFACE_H_
#define __F_MEDIA_INTERFACE_H_

#include <util/string.h>

namespace media
{

typedef enum interface
{
	NONE,
	SOURCE,
	DEMUX,
	DECODE,
	OUTPUT,
	EFFECT,
	ENCODE,
	MUX,
	SINK		
} interface_t;

class Interface
{
	public:
		Interface(){};
};

class SourceInterface : public Interface
{
	public:
		SourceInterface(){};
		virtual ~SourceInterface(){};

		virtual interface_t GetInputInterface( void )
		{
			return SOURCE;
		};

		virtual status_t OpenUri( os::String cUri )
		{
			return ENOSYS;
		};
};

class DemuxInterface : public Interface
{
	public:
		DemuxInterface(){};
		virtual ~DemuxInterface(){};

		virtual interface_t GetInputInterface( void )
		{
			return DEMUX;
		};

		virtual void GetInputMimeType( os::String &cFormat )
		{
			cFormat = "";
			return;
		};

};

class DecodeInterface : public Interface
{
	public:
		DecodeInterface();
		virtual ~DecodeInterface();

		virtual interface_t GetInputInterface( void )
		{
			return DECODE;
		};

		virtual void GetInputMimeType( os::String &cFormat )
		{
			cFormat = "";
			return;
		};
};

}

#endif	/* __F_MEDIA_INTERFACE_H_ */

