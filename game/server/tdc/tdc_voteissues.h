//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef TDC_VOTEISSUES_H
#define TDC_VOTEISSUES_H

#ifdef _WIN32
#pragma once
#endif

#include "vote_controller.h"

//-----------------------------------------------------------------------------
// Purpose: Kick vote
//-----------------------------------------------------------------------------
class CKickIssue : public CBaseIssue
{
public:
	CKickIssue( const char *pszTypeString );

	enum
	{
		KICK_REASON_NONE,
		KICK_REASON_CHEATING,
		KICK_REASON_IDLE,
		KICK_REASON_SCAMMING,
		KICK_REASON_COUNT
	};

	virtual const char		*GetDisplayString( void );
	virtual const char		*GetVotePassedString( void );
	virtual const char		*GetDetailsString( void );
	virtual void			ListIssueDetails( CBasePlayer *pForWhom );
	virtual bool			IsEnabled( void );
	virtual void			OnVoteStarted( void );
	virtual void			OnVoteFailed( int iEntityHoldingVote );
	virtual bool			CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime );
	virtual void			ExecuteCommand( void );
	virtual bool			IsTeamRestrictedVote( void );

	void					Init( void );
	bool					CreateVoteDataFromDetails( const char *pszDetails );
	virtual void			NotifyGC( bool a2 );
	virtual int				PrintLogData( void );
	CBasePlayer				*GetTargetPlayer( void );

private:
	char			m_szPlayerName[MAX_PLAYER_NAME_LENGTH];
	int				m_iReason;
	CSteamID		m_SteamID;
	char			m_szSteamID[MAX_NETWORKID_LENGTH];
};

//-----------------------------------------------------------------------------
// Purpose: Restart map vote
//-----------------------------------------------------------------------------
class CRestartGameIssue : public CBaseIssue
{
public:
	CRestartGameIssue( const char *pszTypeString );

	virtual const char		*GetDisplayString( void );
	virtual const char		*GetVotePassedString( void );
	virtual void			ListIssueDetails( CBasePlayer *pForWhom );
	virtual bool			IsEnabled( void );
	virtual bool			CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime );
	virtual void			ExecuteCommand( void );
};

//-----------------------------------------------------------------------------
// Purpose: Change map vote
//-----------------------------------------------------------------------------
class CChangeLevelIssue : public CBaseIssue
{
public:
	CChangeLevelIssue( const char *pszTypeString );

	virtual const char		*GetDisplayString( void );
	virtual const char		*GetVotePassedString( void );
	virtual const char		*GetDetailsString( void );
	virtual void			ListIssueDetails( CBasePlayer *pForWhom );
	virtual bool			IsEnabled( void );
	virtual bool			IsYesNoVote( void );
	virtual bool			CanTeamCallVote( int iTeam );
	virtual bool			CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime );
	virtual void			ExecuteCommand( void );
};

//-----------------------------------------------------------------------------
// Purpose: Next map vote
//-----------------------------------------------------------------------------
class CNextLevelIssue : public CBaseIssue
{
public:
	CNextLevelIssue( const char *pszTypeString );

	virtual const char		*GetDisplayString( void );
	virtual const char		*GetVotePassedString( void );
	virtual const char		*GetDetailsString( void );
	virtual void			ListIssueDetails( CBasePlayer *pForWhom );
	virtual bool			IsEnabled( void );
	virtual bool			IsYesNoVote( void );
	virtual bool			CanTeamCallVote( int iTeam );
	virtual bool			CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime );
	virtual int				GetNumberVoteOptions( void );
	virtual float			GetQuorumRatio( void );
	virtual void			ExecuteCommand( void );
	virtual bool			GetVoteOptions( CUtlVector <const char*> &vecNames );

	bool					IsInMapChoicesMode( void );

private:
	CUtlStringList			m_MapList;
};

//-----------------------------------------------------------------------------
// Purpose: Extend map vote
//-----------------------------------------------------------------------------
class CExtendLevelIssue : public CBaseIssue
{
public:
	CExtendLevelIssue( const char *pszTypeString );

	virtual const char		*GetDisplayString( void );
	virtual const char		*GetVotePassedString( void );
	virtual void			ListIssueDetails( CBasePlayer *pForWhom );
	virtual bool			IsEnabled( void );
	virtual bool			CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime );
	virtual float			GetQuorumRatio( void );
	virtual void			ExecuteCommand( void );
};

//-----------------------------------------------------------------------------
// Purpose: Scramble teams vote
//-----------------------------------------------------------------------------
class CScrambleTeams : public CBaseIssue
{
public:
	CScrambleTeams( const char *pszTypeString );

	virtual const char		*GetDisplayString( void );
	virtual const char		*GetVotePassedString( void );
	virtual void			ListIssueDetails( CBasePlayer *pForWhom );
	virtual bool			IsEnabled( void );
	virtual bool			CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime );
	virtual void			ExecuteCommand( void );
};

#endif // TDC_INVENTORY_H
