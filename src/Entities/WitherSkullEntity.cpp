
//  WitherSkullEntity.cpp

// Implements the cWitherSkullEntity class representing the entity used by both blue and black wither skulls

#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "WitherSkullEntity.h"
#include "../World.h"





cWitherSkullEntity::cWitherSkullEntity(cEntity * a_Creator, Vector3d a_Pos, Vector3d a_Speed):
	Super(pkWitherSkull, a_Creator, a_Pos, 0.25, 0.25)
{
	SetSpeed(a_Speed);
	SetGravity(0);
	SetAirDrag(0);
}





void cWitherSkullEntity::OnHitSolidBlock(Vector3d a_HitPos, eBlockFace a_HitFace)
{
	m_World->DoExplosionAt(3.0, a_HitPos.x, a_HitPos.y, a_HitPos.z, false, esWitherSkull, this);
	Destroy();
}





void cWitherSkullEntity::OnHitEntity(cEntity & a_EntityHit, Vector3d a_HitPos)
{
	// TODO: If entity is Ender Crystal, destroy it
	a_EntityHit.TakeDamage(dtRangedAttack, this, 0, 1);
	m_World->DoExplosionAt(3.0, a_HitPos.x, a_HitPos.y, a_HitPos.z, false, esWitherSkull, this);

	Destroy();
}




