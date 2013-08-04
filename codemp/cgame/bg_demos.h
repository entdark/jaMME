#include "qcommon/q_shared.h"
#include "game/bg_public.h"

void demoCommandValue( const char *cmd, float * oldVal );

//I brought BG_XMLParse_t and BG_XMLParseBlock_t to q_shared because VS always told me about redeclaration :s

qboolean BG_XMLError(BG_XMLParse_t *parse, const char *msg, ... );
void BG_XMLWarning(BG_XMLParse_t *parse, const char *msg, ... );
qboolean BG_XMLOpen( BG_XMLParse_t *parse, const char *fileName );
qboolean BG_XMLParse( BG_XMLParse_t *parse, const BG_XMLParseBlock_t *fromBlock, const BG_XMLParseBlock_t *parseBlock, void *data );