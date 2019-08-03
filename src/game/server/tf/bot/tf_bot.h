/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * https://github.com/sigsegv-mvm/mvm-reversed/tree/master/server/tf/bot
 *
 * additional reversing by Deathreus
 * created from build 4783668
 */
#ifndef TF_BOT_H
#define TF_BOT_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBot/Player/NextBotPlayer.h"
#include "tf_player.h"
#include "GameEventListener.h"
#include "nav_mesh/tf_nav_area.h"
#include "tf_path_follower.h"

class CTeamControlPoint;
class CCaptureFlag;
class CCaptureZone;
class CTFBotSquad;

class CClosestTFPlayer
{
public:
	CClosestTFPlayer( Vector start )
		: m_vecOrigin( start )
	{
		m_flMinDist = FLT_MAX;
		m_pPlayer = nullptr;
		m_iTeam = TEAM_ANY;
	}

	CClosestTFPlayer( Vector start, int teamNum )
		: m_vecOrigin( start ), m_iTeam( teamNum )
	{
		m_flMinDist = FLT_MAX;
		m_pPlayer = nullptr;
	}

	inline bool operator()( CBasePlayer *player )
	{
		if ( ( player->GetTeamNumber() == TF_TEAM_RED || player->GetTeamNumber() == TF_TEAM_BLUE )
			 && ( m_iTeam == TEAM_ANY || player->GetTeamNumber() == m_iTeam ) )
		{
			float flDistance = ( m_vecOrigin - player->GetAbsOrigin() ).LengthSqr();
			if ( flDistance < m_flMinDist )
			{
				m_flMinDist = flDistance;
				m_pPlayer = (CTFPlayer *)player;
			}
		}

		return true;
	}

	Vector m_vecOrigin;
	float m_flMinDist;
	CTFPlayer *m_pPlayer;
	int m_iTeam;
};


class CTFBot : public NextBotPlayer<CTFPlayer>, public CGameEventListener
{
	DECLARE_CLASS( CTFBot, NextBotPlayer<CTFPlayer> )
public:

	static CBasePlayer *AllocatePlayerEntity( edict_t *edict, const char *playerName );

	CTFBot( CTFPlayer *player=nullptr );
	virtual ~CTFBot();

	DECLARE_INTENTION_INTERFACE( CTFBot )

	struct DelayedNoticeInfo
	{
		CHandle<CBaseEntity> m_hEnt;
		float m_flWhen;
	};
	void			DelayedThreatNotice( CHandle<CBaseEntity> ent, float delay );
	void			UpdateDelayedThreatNotices( void );

	struct SuspectedSpyInfo
	{
		CHandle<CTFPlayer> m_hSpy;
		CUtlVector<int> m_times;

		void Suspect()
		{
			this->m_times.AddToHead( (int)floor( gpGlobals->curtime ) );
		}

		bool IsCurrentlySuspected()
		{
			if ( this->m_times.IsEmpty() )
			{
				return false;
			}

			extern ConVar tf_bot_suspect_spy_forget_cooldown;
			return ( (float)this->m_times.Head() > ( gpGlobals->curtime - tf_bot_suspect_spy_forget_cooldown.GetFloat() ) );
		}

		bool TestForRealizing()
		{
			extern ConVar tf_bot_suspect_spy_touch_interval;
			int t_now = (int)floor( gpGlobals->curtime );
			int t_first = t_now - tf_bot_suspect_spy_touch_interval.GetInt();

			for ( int i=this->m_times.Count()-1; i >= 0; --i )
			{
				if ( this->m_times[i] <= t_first )
				{
					this->m_times.Remove( i );
				}
			}

			this->m_times.AddToHead( t_now );

			CUtlVector<bool> checks;

			checks.SetCount( tf_bot_suspect_spy_touch_interval.GetInt() );
			for ( int i=0; i<checks.Count(); ++i )
			{
				checks[i] = false;
			}

			for ( int i=0; i<this->m_times.Count(); ++i )
			{
				int idx = t_now - this->m_times[i];
				if ( checks.IsValidIndex( idx ) )
				{
					checks[idx] = true;
				}
			}

			bool realized = true;
			for ( int i=0; i<checks.Count(); ++i )
			{
				if ( !checks[i] )
				{
					realized = false;
					break;
				}
			}

			return realized;
		}
	};
	SuspectedSpyInfo *IsSuspectedSpy( CTFPlayer *spy );
	void			SuspectSpy( CTFPlayer *spy );
	void			StopSuspectingSpy( CTFPlayer *spy );
	bool			IsKnownSpy( CTFPlayer *spy ) const;
	void			RealizeSpy( CTFPlayer *spy );
	void			ForgetSpy( CTFPlayer *spy );

	virtual void	Spawn( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual void	UpdateOnRemove( void ) override;
	virtual void	FireGameEvent( IGameEvent *event );
	virtual int		GetBotType() const { return 1337; }
	virtual int		DrawDebugTextOverlays( void );
	virtual void	PhysicsSimulate( void );
	virtual void	Touch( CBaseEntity *other );

	virtual CTFNavArea *GetLastKnownArea( void ) const override;

	virtual bool	IsDormantWhenDead( void ) const { return false; }
	virtual bool	IsDebugFilterMatch( const char *name ) const;

	virtual void	PressFireButton( float duration = -1.0f );
	virtual void	PressAltFireButton( float duration = -1.0f );
	virtual void	PressSpecialFireButton( float duration = -1.0f );

	virtual void	AvoidPlayers( CUserCmd *pCmd );

	virtual CBaseCombatCharacter *GetEntity( void ) const;

	virtual bool	IsAllowedToPickUpFlag( void );

	void			DisguiseAsEnemy( void );

	bool			IsCombatWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsQuietWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsHitScanWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsExplosiveProjectileWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsContinuousFireWeapon( CTFWeaponBase *weapon = nullptr ) const;
	bool			IsBarrageAndReloadWeapon( CTFWeaponBase *weapon = nullptr ) const;

	float			GetMaxAttackRange( void ) const;
	float			GetDesiredAttackRange( void ) const;

	float			GetDesiredPathLookAheadRange( void ) const;

	bool			ShouldFireCompressionBlast( void );

	CTFNavArea*		FindVantagePoint( float flMaxDist );

	virtual ILocomotion *GetLocomotionInterface( void ) const override { return m_locomotor; }
	virtual IBody *GetBodyInterface( void ) const override { return m_body; }
	virtual IVision *GetVisionInterface( void ) const override { return m_vision; }

	bool			IsLineOfFireClear( CBaseEntity *to ) const;
	bool			IsLineOfFireClear( const Vector& to ) const;
	bool			IsLineOfFireClear( const Vector& from, CBaseEntity *to ) const;
	bool			IsLineOfFireClear( const Vector& from, const Vector& to ) const;
	bool			IsAnyEnemySentryAbleToAttackMe( void ) const;
	bool			IsThreatAimingTowardsMe( CBaseEntity *threat, float dotTolerance = 0.8 ) const;
	bool			IsThreatFiringAtMe( CBaseEntity *threat ) const;

	bool			IsEntityBetweenTargetAndSelf( CBaseEntity *blocker, CBaseEntity *target );

	bool			IsAmmoLow( void ) const;
	bool			IsAmmoFull( void ) const;

	bool			AreAllPointsUncontestedSoFar( void ) const;
	bool			IsNearPoint( CTeamControlPoint *point ) const;
	void			ClearMyControlPoint( void ) { m_hMyControlPoint = nullptr; }
	CTeamControlPoint *GetMyControlPoint( void );
	bool			IsAnyPointBeingCaptured( void ) const;
	bool			IsPointBeingContested( CTeamControlPoint *point ) const;
	float			GetTimeLeftToCapture( void );
	CTeamControlPoint *SelectPointToCapture( const CUtlVector<CTeamControlPoint *> &candidates );
	CTeamControlPoint *SelectPointToDefend( const CUtlVector<CTeamControlPoint *> &candidates );
	CTeamControlPoint *SelectClosestPointByTravelDistance( const CUtlVector<CTeamControlPoint *> &candidates ) const;

	CCaptureZone*	GetFlagCaptureZone( void );
	CCaptureFlag*	GetFlagToFetch( void );

	float			TransientlyConsistentRandomValue( float duration, int seed ) const;

	Action<CTFBot> *OpportunisticallyUseWeaponAbilities( void );

	CBaseObject*	GetNearestKnownSappableTarget( void ) const;

	void			UpdateLookingAroundForEnemies( void );
	void			UpdateLookingForIncomingEnemies( bool );

	bool			EquipBestWeaponForThreat( const CKnownEntity *threat );
	bool			EquipLongRangeWeapon( void );

	void			PushRequiredWeapon( CTFWeaponBase *weapon );
	bool			EquipRequiredWeapon( void );
	void			PopRequiredWeapon( void );

	CTFBotSquad*	GetSquad( void ) const { return m_pSquad; }
	bool			IsSquadmate( CTFPlayer *player ) const;
	void			JoinSquad( CTFBotSquad *squad );
	void			LeaveSquad();

	struct SniperSpotInfo
	{	// TODO: Better names
		CTFNavArea *m_area1;
		Vector m_pos1;
		CTFNavArea *m_area2;
		Vector m_pos2;
		float m_length;
		float m_difference;
	};
	void			AccumulateSniperSpots( void );
	void			SetupSniperSpotAccumulation( void );
	void			ClearSniperSpots( void );
	const CUtlVector<SniperSpotInfo> *GetSniperSpots( void ) const { return &m_sniperSpots; }

	void			SelectReachableObjects( CUtlVector<EHANDLE> const& append, CUtlVector<EHANDLE> *outVector, INextBotFilter const& func, CNavArea *pStartArea, float flMaxRange );
	CTFPlayer *		SelectRandomReachableEnemy( void );

	bool			CanChangeClass( void );
	const char*		GetNextSpawnClassname( void );

	float			GetUberDeployDelayDuration( void ) const;
	float			GetUberHealthThreshold( void ) const;

	friend class CTFBotSquad;
	CTFBotSquad *m_pSquad;
	float m_flFormationError;
	bool m_bIsInFormation;

	CTFNavArea *m_HomeArea;

	CHandle<CBaseObject> m_hTargetSentry;
	Vector m_vecLastHurtBySentry;

	enum DifficultyType
	{
		EASY   = 0,
		NORMAL = 1,
		HARD   = 2,
		EXPERT = 3
	};
	int m_iSkill;

	bool m_bLookingAroundForEnemies;

	CountdownTimer m_cpChangedTimer;

private:
	CountdownTimer m_lookForEnemiesTimer;

	ILocomotion *m_locomotor;
	IBody *m_body;
	IVision *m_vision;

	CTFPlayer *m_controlling;

	CHandle<CTeamControlPoint> m_hMyControlPoint;
	CountdownTimer m_myCPValidDuration;

	CHandle<CCaptureZone> m_hMyCaptureZone;

	CountdownTimer m_useWeaponAbilityTimer;

	CUtlVectorConservative< CHandle<CTFWeaponBase> > m_requiredEquipStack;

	CUtlVector<DelayedNoticeInfo> m_delayedThreatNotices;

	CUtlVectorAutoPurge<SuspectedSpyInfo *> m_suspectedSpies;
	CUtlVector< CHandle<CTFPlayer> > m_knownSpies;

	CUtlVector<SniperSpotInfo> m_sniperSpots;
	CUtlVector<CTFNavArea *> m_sniperAreasRed;
	CUtlVector<CTFNavArea *> m_sniperAreasBlu;
	CBaseEntity *m_sniperGoalEnt;
	Vector m_sniperGoal;
	CountdownTimer m_sniperSpotTimer;
};

class CTFBotPathCost : public IPathCost
{
public:
	CTFBotPathCost( CTFBot *actor, RouteType routeType = DEFAULT_ROUTE );
	virtual ~CTFBotPathCost() { }

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const override;

private:
	CTFBot *m_Actor;
	RouteType m_iRouteType;
	float m_flStepHeight;
	float m_flMaxJumpHeight;
	float m_flDeathDropHeight;
};

inline CTFBot *ToTFBot( CBaseEntity *ent )
{
	if ( !ent || !ent->IsPlayer() )
		return NULL;

	if ( !( (CBasePlayer *)ent )->IsBotOfType( 1337 ) )
		return NULL;

	Assert( dynamic_cast<CTFBot *>( ent ) );
	return static_cast<CTFBot *>( ent );
}

#endif