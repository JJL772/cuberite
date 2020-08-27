
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "Wither.h"

#include "../World.h"
#include "../Entities/Player.h"
#include "../Entities/WitherSkullEntity.h"





cWither::cWither(void) :
	Super("Wither", mtWither, "entity.wither.hurt", "entity.wither.death", "entity.wither.ambient", 0.9, 4.0),
	m_WitherInvulnerableTicks(220),
	m_RemainingTimeBetweenShots(witherTicksBetweenShots),
	m_RemainingWanderTicks(witherWanderTicks),
	m_HadTarget(false)
{
	SetMaxHealth(300);
	SetHealth(GetMaxHealth() / 3);
	SetGravity(0);
	SetAirDrag(0);
}





bool cWither::IsArmored(void) const
{
	return GetHealth() <= (GetMaxHealth() / 2);
}





bool cWither::DoTakeDamage(TakeDamageInfo & a_TDI)
{
	if (a_TDI.DamageType == dtDrowning)
	{
		return false;
	}

	if (m_WitherInvulnerableTicks > 0)
	{
		return false;
	}

	if (IsArmored() && (a_TDI.DamageType == dtRangedAttack))
	{
		return false;
	}

	return Super::DoTakeDamage(a_TDI);
}





void cWither::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	Super::Tick(a_Dt, a_Chunk);

	SetAirDrag(0);
	if (!IsTicking())
	{
		// The base class tick destroyed us
		return;
	}

	if(m_HadTarget)
	{
		m_RemainingWanderTicks = 1;
		m_HadTarget = false;
	}

	/* Called while the wither is invulnerable */
	if (m_WitherInvulnerableTicks > 0)
	{
		unsigned int NewTicks = m_WitherInvulnerableTicks - 1;

		if (NewTicks == 0)
		{
			// HACK: So we dont get yeeted by the explosion
			SetAirDrag(10000000);
			m_World->DoExplosionAt(7.0, GetPosX(), GetPosY(), GetPosZ(), false, esWitherBirth, this);
		}

		m_WitherInvulnerableTicks = NewTicks;

		if ((NewTicks % 10) == 0)
		{
			Heal(10);
		}
		SetSpeedY(1);
	}

	/* Actual code to run */
	if(m_WitherInvulnerableTicks == 0 && m_RemainingTimeBetweenShots-- == 0)
	{
		m_RemainingTimeBetweenShots = witherTicksBetweenShots;


		/* Targeting code */
		double minDistance = (double)witherTargetRange;
		cPawn* pawn = nullptr;

		/* Get the distance between the entity and the wither */
		/* If we've already got a target that's still in range, we should just keep focusing on that */
		cPawn* curTarget = GetTarget();
		if(curTarget && GetPosition().Distance(curTarget->GetPosition()) < witherTargetRange)
		{

		}
		else
		{
			m_World->ForEachEntityInBox(
				cBoundingBox(GetPosition(), witherTargetRange),
				[this, &minDistance, &pawn](cEntity & ent) -> bool {
					if (ent.IsPawn())
					{

						double dist = fabs(
							this->GetPosition().Distance(ent.GetPosition()));
						if (dist < minDistance && dist != 0.0)
						{
							if (ent.IsPlayer())
							{
								cPlayer * player = (cPlayer *)&ent;
								if (player->GetGameMode() ==
										eGameMode_Spectator ||
									player->GetGameMode() == eGameMode_Creative)
									return false;
							}
							pawn = (cPawn *)&ent;
							minDistance = dist;
						}
					}
					return false;
				});
		}

		if(pawn)
		{
			this->SetTarget(pawn);
			m_HadTarget = true;
		}
	}

	if(m_WitherInvulnerableTicks <= 0)
	{
		auto& random = GetRandomProvider();
		/* No target? Wander around a bit */
		if(!GetTarget())
		{
			if(m_RemainingWanderTicks-- == 0)
			{
				double yaw = random.RandInt(-180, 180);
				SetYaw(random.RandInt(-180, 180));
				SetSpeed(GetLookVector() * random.RandReal(9.0, 10.0));
				m_RemainingWanderTicks = witherWanderTicks + random.RandInt(-5, 40);
			}
		}
		else
		{
			/* Check to make sure we're not within a cylinder exclusion zone around the entity. If we end up directly above the entity, we freak out, so avoid that! */
			/* if we are within that zone, go ahead set our speed to 0 */
			double dx = fabs(GetTarget()->GetPosX() - GetPosX());
			double dy = fabs(GetTarget()->GetPosZ() - GetPosZ());
			double dist = sqrt(dx*dx + dy*dy);

			if(dist < 4.0)
			{
				SetSpeed(GetLookVector() * random.RandReal(9.0, 10.0));
			}
			else
			{
				/* If we are within that exclusion zone, stop! */
				SetSpeedX(0.0);
				SetSpeedZ(0.0);
			}

			/* Attacking code */
			if(m_RemainingTimeBetweenShots-- == 0)
			{
				auto witherSkull = std::make_unique<cWitherSkullEntity>(this, GetPosition(), GetLookVector() * 20);
				auto witherSkullPtr = witherSkull.get();
				witherSkullPtr->Initialize(std::move(witherSkull), *m_World);


				m_World->BroadcastSoundEffect("entity.wither.shoot", GetPosition(), 4.0f, 1.0f);

				m_RemainingTimeBetweenShots = witherTicksBetweenShots * random.RandReal(0.6, 1.2);
			}
		}
	}

	/* Height governor */
	double height = GetPosY() - m_World->GetHeight(GetPosX(), GetPosZ());
	if(height > (witherTargetHeight + witherHeightDev))
		SetSpeedY(-3);
	else if(height < (witherTargetHeight - witherHeightDev))
		SetSpeedY(3);


	m_World->BroadcastEntityMetadata(*this);
}





void cWither::GetDrops(cItems & a_Drops, cEntity * a_Killer)
{
	AddRandomDropItem(a_Drops, 1, 1, E_ITEM_NETHER_STAR);
}





void cWither::KilledBy(TakeDamageInfo & a_TDI)
{
	Super::KilledBy(a_TDI);

	Vector3d Pos = GetPosition();
	m_World->ForEachPlayer([=](cPlayer & a_Player)
		{
			// TODO 2014-05-21 xdot: Vanilla minecraft uses an AABB check instead of a radius one
			double Dist = (a_Player.GetPosition() - Pos).Length();
			if (Dist < 50.0)
			{
				// If player is close, award achievement
				a_Player.AwardAchievement(Statistic::AchKillWither);
			}
			return false;
		}
	);
}




