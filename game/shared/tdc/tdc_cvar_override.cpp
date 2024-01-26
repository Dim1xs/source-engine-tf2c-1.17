#include "cbase.h"
#include "fmtstr.h"

#ifdef GAME_DLL
#include "gameinterface.h"
extern CServerGameDLL g_ServerGameDLL;
#endif


enum ETDCCVOverrideFlags
{
	OVERRIDE_SERVER = (1 << 0),
	OVERRIDE_CLIENT = (1 << 1),

	OVERRIDE_BOTH = (OVERRIDE_SERVER | OVERRIDE_CLIENT),
};

struct CTDCConVarDefaultOverrideEntry
{
	ETDCCVOverrideFlags nFlags; // flags
	const char *pszName;       // ConVar name
	const char *pszValue;      // new default value
};

static CTDCConVarDefaultOverrideEntry s_TFConVarOverrideEntries[] =
{
	/* interpolation */
	{ OVERRIDE_BOTH,   "sv_client_min_interp_ratio",   "1" },
	{ OVERRIDE_BOTH,   "sv_client_max_interp_ratio",   "2" },

	/* rates */
	{ OVERRIDE_BOTH,   "cl_updaterate",               "66" },
	{ OVERRIDE_BOTH,   "cl_cmdrate",                  "66" },
	{ OVERRIDE_BOTH,   "rate",                     "80000" },

	/* voice */
	{ OVERRIDE_SERVER, "sv_voicecodec",      "vaudio_celt" },
	{ OVERRIDE_CLIENT, "voice_maxgain",                "1" },

	/* download/upload */
	{ OVERRIDE_SERVER, "sv_allowupload",               "0" },
};


// override some of the more idiotic ConVar defaults with more reasonable values
class CTDCConVarDefaultOverride : public CAutoGameSystem
{
public:
	CTDCConVarDefaultOverride() : CAutoGameSystem("TFConVarDefaultOverride") {}
	virtual ~CTDCConVarDefaultOverride() {}

	virtual bool Init() OVERRIDE;

private:
	void OverrideDefault(const CTDCConVarDefaultOverrideEntry& entry);

#ifdef GAME_DLL
	static const char *DLLName() { return "SERVER"; }
#else
	static const char *DLLName() { return "CLIENT"; }
#endif
};
static CTDCConVarDefaultOverride s_TFConVarOverride;


bool CTDCConVarDefaultOverride::Init()
{
	for ( const auto& entry : s_TFConVarOverrideEntries )
	{
		OverrideDefault( entry );
	}

#ifdef GAME_DLL
	/* ensure that server-side clock correction is ACTUALLY limited to 2 tick-intervals */
	static CFmtStrN<16> s_Buf( "%.0f", 2 * ( g_ServerGameDLL.GetTickInterval() * 1000.0f ) );
	OverrideDefault( { OVERRIDE_SERVER, "sv_clockcorrection_msecs", s_Buf } );
#endif

	return true;
}


void CTDCConVarDefaultOverride::OverrideDefault(const CTDCConVarDefaultOverrideEntry& entry)
{
#ifdef GAME_DLL
	if ( !( entry.nFlags & OVERRIDE_SERVER ) )
		return;
#else
	if ( !( entry.nFlags & OVERRIDE_CLIENT ) )
		return;
#endif

	ConVarRef ref( entry.pszName, true );
	if ( !ref.IsValid() )
	{
		DevWarning( "[%s] CTDCConVarDefaultOverride: can't get a valid ConVarRef for \"%s\"\n", DLLName(), entry.pszName );
		return;
	}

	auto pConVar = dynamic_cast<ConVar *>( ref.GetLinkedConVar() );
	if ( pConVar == nullptr )
	{
		DevWarning( "[%s] CTDCConVarDefaultOverride: can't get a ConVar ptr for \"%s\"\n", DLLName(), entry.pszName );
		return;
	}

	CUtlString strOldDefault( pConVar->GetDefault() );
	CUtlString strOldValue  ( pConVar->GetString()  );

	/* override the convar's default, and if it was at the default, keep it there */
	bool bWasDefault = FStrEq( pConVar->GetString(), pConVar->GetDefault() );
	pConVar->SetDefault( entry.pszValue );
	if ( bWasDefault )
		pConVar->Revert();

	DevMsg( "[%s] CTDCConVarDefaultOverride: \"%s\" was \"%s\"/\"%s\", now \"%s\"/\"%s\"\n", DLLName(), entry.pszName,
		strOldDefault.Get(), strOldValue.Get(), pConVar->GetDefault(), pConVar->GetString() );
}
