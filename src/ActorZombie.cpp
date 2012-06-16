#include "monsterIncludeFiles.h"

#include "Log.h"
#include "ActorFactory.h"
#include "Blood.h"

bool Zombie::actorSpecificAct() {
	return attemptResurrect();
}

bool MajorClaphamLee::actorSpecificAct() {
	if(attemptResurrect()) {
		return true;
	}

	if(deadState == actorDeadState_alive) {
		if(playerAwarenessCounter > 0) {
			if(hasSummonedTombLegions == false) {

				bool blockers[MAP_X_CELLS][MAP_Y_CELLS];
				eng->mapTests->makeVisionBlockerArray(blockers);

				if(checkIfSeeActor(*(eng->player), blockers)) {
					eng->mapTests->makeMoveBlockerArray(this, blockers);
					eng->basicUtils->reverseBoolArray(blockers);
					vector<coord> freeCells;
					eng->mapTests->makeMapVectorFromArray(blockers, freeCells);
					sort(freeCells.begin(), freeCells.end(), IsCloserToOrigin(pos, eng));

					const unsigned int NR_OF_SPAWNS = 5;
					if(freeCells.size() >= NR_OF_SPAWNS + 1) {
						eng->log->addMessage("Major Clapham Lee calls forth his Tomb-Legions!");
						eng->player->shock(shockValue_heavy, 0);
						for(unsigned int i = 0; i < NR_OF_SPAWNS; i++) {
							if(i == 0) {
								Monster* monster = dynamic_cast<Monster*>(eng->actorFactory->spawnActor(actor_deanHalsey, freeCells.at(0)));
								monster->playerAwarenessCounter = 999;
								monster->leader = this;
								freeCells.erase(freeCells.begin());
							} else {
								const int ZOMBIE_TYPE = eng->dice.getInRange(0, 2);
								ActorDevNames_t devName = actor_zombie;
								switch(ZOMBIE_TYPE) {
								case 0:
									devName = actor_zombie;
									break;
								case 1:
									devName = actor_zombieAxe;
									break;
								case 2:
									devName = actor_bloatedZombie;
									break;
								}
								Monster* monster = dynamic_cast<Monster*>(eng->actorFactory->spawnActor(devName, freeCells.at(0)));
								monster->playerAwarenessCounter = 999;
								monster->leader = this;
								freeCells.erase(freeCells.begin());
							}
						}
						eng->renderer->drawMapAndInterface();
						hasSummonedTombLegions = true;
						eng->gameTime->letNextAct();
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool Zombie::attemptResurrect() {
	if(deadState == actorDeadState_corpse) {
		if(hasResurrected == false) {
			deadTurnCounter += 1;
			if(deadTurnCounter > 5) {
				if(pos != eng->player->pos && eng->dice(1, 100) <= 7) {
					deadState = actorDeadState_alive;
					m_instanceDefinition.HP = (getHP_max() * 3) / 4;
					m_instanceDefinition.glyph = m_archetypeDefinition->glyph;
					m_instanceDefinition.tile = m_archetypeDefinition->tile;
					m_instanceDefinition.color = m_archetypeDefinition->color;
					hasResurrected = true;
					m_instanceDefinition.nrOfKills--;
					m_archetypeDefinition->nrOfKills--;
					if(eng->map->playerVision[pos.x][pos.y] == true) {
						eng->log->addMessage(getNameThe() + " rises again!!");
						eng->player->shock(shockValue_some, 0);
					}

					playerAwarenessCounter = m_instanceDefinition.nrTurnsAwarePlayer * 2;
					eng->gameTime->letNextAct();
					return true;
				}
			}
		}
	}
	return false;
}

void Zombie::actorSpecificDie() {
	//If resurrected once and has corpse, blow up the corpse
	if(hasResurrected && deadState == actorDeadState_corpse) {
		deadState = actorDeadState_mangled;
		eng->gore->makeBlood(pos);
		eng->gore->makeGore(pos);
	}
}

void ZombieClaw::actorSpecific_spawnStartItems() {
	Item* item = NULL;
	if(eng->dice(1, 100) < 30) {
		item = eng->itemFactory->spawnItem(item_zombieClawDiseased);
	} else {
		item = eng->itemFactory->spawnItem(item_zombieClaw);
	}
	m_inventory->putItemInIntrinsics(item);
}

void ZombieAxe::actorSpecific_spawnStartItems() {
	m_inventory->putItemInIntrinsics(eng->itemFactory->spawnItem(item_zombieAxe));
}

void BloatedZombie::actorSpecific_spawnStartItems() {
	m_inventory->putItemInIntrinsics(eng->itemFactory->spawnItem(item_bloatedZombiePunch));
	m_inventory->putItemInIntrinsics(eng->itemFactory->spawnItem(item_bloatedZombieSpit));
}
