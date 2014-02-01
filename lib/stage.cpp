#include <stage.h>
#include <buffer.h>
#include <packet.h>

using namespace os;
using namespace media;

Stage::Stage()
{
}

Stage::~Stage()
{
}

String Stage::GetName( void )
{
	return "Unknown";
}

status_t Stage::GetPacket( Packet **ppcPacket, int nInterface )
{
	return ENOSYS;
}

InputStage::InputStage()
{
}

InputStage::~InputStage()
{
}

int InputStage::GetOutputCount( void )
{
	return 0;
}

status_t InputStage::Connect( Buffer *pcBuffer )
{
	return ENOSYS;
}

