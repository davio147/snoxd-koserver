#include "stdafx.h"
#include "MagicProcess.h"
#include "ServerDlg.h"
#include "User.h"
#include "Npc.h"
#include "Region.h"

extern CRITICAL_SECTION g_region_critical;

CMagicProcess::CMagicProcess()
{
	m_pSrcUser = NULL;
	m_bMagicState = NONE;
}

CMagicProcess::~CMagicProcess()
{

}

void CMagicProcess::MagicPacket(Packet & pkt)
{
	int magicid = 0;
	int16 sid, tid;
	uint16 data1 = 0, data2 = 0, data3 = 0, data4 = 0, data5 = 0, data6 = 0, TotalDex=0, righthand_damage = 0, result = 1;
	_MAGIC_TABLE* pTable = NULL;

	sid = m_pSrcUser->m_iUserId;

	BYTE command = pkt.read<uint8>();
	pkt >> tid >> magicid >> data1 >> data2 >> data3 >> data4 >> data5 >> data6 >> TotalDex >> righthand_damage;
	//TRACE("MagicPacket - command=%d, tid=%d, magicid=%d\n", command, tid, magicid);

	pTable = IsAvailable( magicid, tid, command );     // If magic was successful.......
	if( !pTable ) return;

	if( command == MAGIC_EFFECTING )     // Is target another player? 
	{
		switch( pTable->bType[0] ) {
		case 1:
			result = ExecuteType1( pTable->iNum, tid, data1, data2, data3, 1 );
			break;
		case 2:
			result = ExecuteType2( pTable->iNum, tid, data1, data2, data3 );	
			break;
		case 3:
			ExecuteType3( pTable->iNum, tid, data1, data2, data3, pTable->bMoral, TotalDex, righthand_damage );
			break;
		case 4:
			ExecuteType4( pTable->iNum, sid, tid, data1, data2, data3, pTable->bMoral );
			break;
		case 5:
			ExecuteType5( pTable->iNum );
			break;
		case 6:
			ExecuteType6( pTable->iNum );
			break;
		case 7:
			ExecuteType7( pTable->iNum );
			break;
		case 8:
			ExecuteType8( pTable->iNum );
			break;
		case 9:
			ExecuteType9( pTable->iNum );
			break;
		}

		if(result != 0)	{
			switch( pTable->bType[1] ) {
			case 1:
				ExecuteType1( pTable->iNum, tid, data4, data5, data6, 2 );
				break;
			case 2:
				ExecuteType2( pTable->iNum, tid, data1, data2, data3 );	
				break;
			case 3:
				ExecuteType3( pTable->iNum, tid, data1, data2, data3, pTable->bMoral, TotalDex, righthand_damage );
				break;
			case 4:
				ExecuteType4( pTable->iNum, sid, tid, data1, data2, data3, pTable->bMoral );
				break;
			case 5:
				ExecuteType5( pTable->iNum );
				break;
			case 6:
				ExecuteType6( pTable->iNum );
				break;
			case 7:
				ExecuteType7( pTable->iNum );
				break;
			case 8:
				ExecuteType8( pTable->iNum );
				break;
			case 9:
				ExecuteType9( pTable->iNum );
				break;
			}
		}
	}
}

_MAGIC_TABLE* CMagicProcess::IsAvailable(int magicid, int tid, BYTE type )
{
	if (m_pSrcUser == NULL)
		return NULL;

	_MAGIC_TABLE *pTable = g_pMain.m_MagictableArray.GetData(magicid);
	if (!pTable)
		m_bMagicState = NONE;

	return pTable;
}

BYTE CMagicProcess::ExecuteType1(int magicid, int tid, int data1, int data2, int data3, BYTE sequence)   // Applied to an attack skill using a weapon.
{	
	int damage = 0;
	BYTE bResult = 1;     // Variable initialization. result == 1 : success, 0 : fail
	_MAGIC_TABLE* pMagic = g_pMain.m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return 0; 

	damage = m_pSrcUser->GetDamage(tid, magicid);  // Get damage points of enemy.	
// 	if(damage <= 0)	damage = 1;
	//TRACE("magictype1 ,, magicid=%d, damage=%d\n", magicid, damage);

//	if (damage > 0) {
		CNpc* pNpc = NULL ;      // Pointer initialization!
		pNpc = g_pMain.m_arNpc.GetData(tid-NPC_BAND);
		if(pNpc == NULL || pNpc->m_NpcState == NPC_DEAD || pNpc->m_iHP == 0)	{
			bResult = 0;
			goto packet_send;
		}

		if(pNpc->SetDamage(magicid, damage, m_pSrcUser->m_strUserID, m_pSrcUser->m_iUserId + USER_BAND) == FALSE)	{
			// Npc가 죽은 경우,,
			pNpc->SendExpToUserList(); // 경험치 분배!!
			pNpc->SendDead();
			//m_pSrcUser->SendAttackSuccess(tid, MAGIC_ATTACK_TARGET_DEAD, 0, pNpc->m_iHP);
			m_pSrcUser->SendAttackSuccess(tid, ATTACK_TARGET_DEAD, damage, pNpc->m_iHP);
		}
		else	{
			// 공격 결과 전송
			m_pSrcUser->SendAttackSuccess(tid, ATTACK_SUCCESS, damage, pNpc->m_iHP);
		}

packet_send:
	if (pMagic->bType[1] == 0 || pMagic->bType[1] == 1)
	{
		Packet result(AG_MAGIC_ATTACK_RESULT, uint8(MAGIC_EFFECTING));
		result	<< magicid
				<< m_pSrcUser->m_iUserId << int16(tid) 
				<< int16(data1) << uint8(bResult) << int16(data3)
				<< int16(0) << int16(0) << int16(0)
				<< int16(damage == 0 ? -104 : 0);
		g_pMain.Send(&result);	
	}

	return bResult;
}

BYTE CMagicProcess::ExecuteType2(int magicid, int tid, int data1, int data2, int data3)
{
	Packet result(AG_MAGIC_ATTACK_RESULT, uint8(MAGIC_EFFECTING));
	int damage = m_pSrcUser->GetDamage(tid, magicid);;
	BYTE bResult = 1;
	
	result << magicid << m_pSrcUser->m_iUserId << tid << data1;

	if (damage > 0){
		CNpc* pNpc = NULL ;      // Pointer initialization!
		pNpc = g_pMain.m_arNpc.GetData(tid-NPC_BAND);
		if(pNpc == NULL || pNpc->m_NpcState == NPC_DEAD || pNpc->m_iHP == 0)	{
			bResult = 0;
			goto packet_send;
		}

		if (!pNpc->SetDamage(magicid, damage, m_pSrcUser->m_strUserID, m_pSrcUser->m_iUserId + USER_BAND))
		{
			result	<< int16(bResult) << data3 << int16(0) << int16(0) << int16(0)
					<< int16(damage == 0 ? -104 : 0);
			g_pMain.Send(&result);

			pNpc->SendExpToUserList(); // 경험치 분배!!
			pNpc->SendDead();
			m_pSrcUser->SendAttackSuccess(tid, MAGIC_ATTACK_TARGET_DEAD, damage, pNpc->m_iHP);
			//m_pSrcUser->SendAttackSuccess(tid, ATTACK_TARGET_DEAD, damage, pNpc->m_iHP);

			return bResult;
		}
		else	{
			// 공격 결과 전송
			m_pSrcUser->SendAttackSuccess(tid, ATTACK_SUCCESS, damage, pNpc->m_iHP);
		}
	}
//	else
//		result = 0;

packet_send:
	// this is a little redundant but leaving it in just in case order is intended
	// this should all be removed eventually anyway...
	result	<< int16(bResult) << data3 << int16(0) << int16(0) << int16(0)
			<< int16(damage == 0 ? -104 : 0);
	g_pMain.Send(&result);
	return bResult;
}

void CMagicProcess::ExecuteType3(int magicid, int tid, int data1, int data2, int data3, int moral, int dexpoint, int righthand_damage )   // Applied when a magical attack, healing, and mana restoration is done.
{	
	int damage = 0, attack_type = 0; 
	BOOL bResult = TRUE;
	_MAGIC_TYPE3* pType = NULL;
	CNpc* pNpc = NULL ;      // Pointer initialization!

	_MAGIC_TABLE* pMagic = NULL;
	pMagic = g_pMain.m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic ) return; 

	if(tid == -1)	{	// 지역 공격
		bResult = AreaAttack(3, magicid, moral, data1, data2, data3, dexpoint, righthand_damage);
		//if(result == 0)		goto packet_send;
		//else 
			return;
	}

	pNpc = g_pMain.m_arNpc.GetData(tid-NPC_BAND);
	if(pNpc == NULL || pNpc->m_NpcState == NPC_DEAD || pNpc->m_iHP == 0)	{
		bResult = FALSE;
		goto packet_send;
	}
	
	pType = g_pMain.m_Magictype3Array.GetData( magicid );      // Get magic skill table type 3.
	if( !pType ) return;
	
//	if (pType->sFirstDamage < 0) {
	if ((pType->sFirstDamage < 0) && (pType->bDirectType == 1) && (magicid < 400000)) {
		damage = GetMagicDamage(tid, pType->sFirstDamage, pType->bAttribute, dexpoint, righthand_damage) ;
	}
	else {
		damage = pType->sFirstDamage ;
	}

	//TRACE("magictype3 ,, magicid=%d, damage=%d\n", magicid, damage);
	
	if (pType->bDuration == 0)    { // Non-Durational Spells.
		if (pType->bDirectType == 1) {    // Health Point related !
			//damage = pType->sFirstDamage;     // Reduce target health point
//			if(damage >= 0)	{
			if(damage > 0)	{
				bResult = pNpc->SetHMagicDamage(damage);
			}
			else	{
				damage = abs(damage);
				if(pType->bAttribute == 3)   attack_type = 3; // 기절시키는 마법이라면.....
				else attack_type = magicid;

				if (!pNpc->SetDamage(attack_type, damage, m_pSrcUser->m_strUserID, m_pSrcUser->m_iUserId + USER_BAND))
				{
					// Npc가 죽은 경우,,
					pNpc->SendExpToUserList(); // 경험치 분배!!
					pNpc->SendDead();
					m_pSrcUser->SendAttackSuccess(tid, MAGIC_ATTACK_TARGET_DEAD, damage, pNpc->m_iHP, MAGIC_ATTACK);
				}
				else	{
					// 공격 결과 전송
					m_pSrcUser->SendAttackSuccess(tid, ATTACK_SUCCESS, damage, pNpc->m_iHP, MAGIC_ATTACK);
				}
			}
		}
		else if ( pType->bDirectType == 2 || pType->bDirectType == 3 )    // Magic or Skill Point related !
			pNpc->MSpChange(pType->bDirectType, pType->sFirstDamage);     // Change the SP or the MP of the target.
		else if( pType->bDirectType == 4 )     // Armor Durability related.
			pNpc->ItemWoreOut( DEFENCE, pType->sFirstDamage);     // Reduce Slot Item Durability
	}
	else if (pType->bDuration != 0)   {  // Durational Spells! Remember, durational spells only involve HPs.
		if(damage >= 0)	{
		}
		else	{
			damage = abs(damage);
			if(pType->bAttribute == 3)   attack_type = 3; // 기절시키는 마법이라면.....
			else attack_type = magicid;
				
			if(pNpc->SetDamage(attack_type, damage, m_pSrcUser->m_strUserID, m_pSrcUser->m_iUserId + USER_BAND) == FALSE)	{
				// Npc가 죽은 경우,,
				pNpc->SendExpToUserList(); // 경험치 분배!!
				pNpc->SendDead();
				m_pSrcUser->SendAttackSuccess(tid, MAGIC_ATTACK_TARGET_DEAD, damage, pNpc->m_iHP);
			}
			else	{
				// 공격 결과 전송
				m_pSrcUser->SendAttackSuccess(tid, ATTACK_SUCCESS, damage, pNpc->m_iHP);
			}
		}

		damage = GetMagicDamage(tid, pType->sTimeDamage, pType->bAttribute, dexpoint, righthand_damage);
		// The duration magic routine.
		for(int i=0; i<MAX_MAGIC_TYPE3; i++)	{
			if(pNpc->m_MagicType3[i].sHPAttackUserID == -1 && pNpc->m_MagicType3[i].byHPDuration == 0)	{
				pNpc->m_MagicType3[i].sHPAttackUserID = m_pSrcUser->m_iUserId;
				pNpc->m_MagicType3[i].tStartTime = UNIXTIME;
				pNpc->m_MagicType3[i].byHPDuration = pType->bDuration;
				pNpc->m_MagicType3[i].byHPInterval = 2;
				pNpc->m_MagicType3[i].sHPAmount = damage / (pType->bDuration / 2);
				break;
			}
		}	
	} 

packet_send:
	//if ( pMagic->bType[1] == 0 || pMagic->bType[1] == 3 ) 
	{
		Packet result(AG_MAGIC_ATTACK_RESULT, uint8(MAGIC_EFFECTING));
		result	<< magicid << m_pSrcUser->m_iUserId << uint16(tid)
				<< uint16(data1) << uint16(bResult) << uint16(data3)
				<< uint16(moral) << uint16(0) << uint16(0);
		g_pMain.Send(&result);
	}
	
}

void CMagicProcess::ExecuteType4(int magicid, int sid, int tid, int data1, int data2, int data3, int moral )
{
	Packet result(AG_MAGIC_ATTACK_RESULT);
	int damage = 0;
	BOOL bResult = TRUE;

	_MAGIC_TYPE4* pType = NULL;
	CNpc* pNpc = NULL ;      // Pointer initialization!

	if(tid == -1)	{	// 지역 공격
		bResult = AreaAttack(4, magicid, moral, data1, data2, data3, 0, 0);
		if (bResult)
			return;

		goto fail_return;
	}

	pNpc = g_pMain.m_arNpc.GetData(tid-NPC_BAND);
	if(pNpc == NULL || pNpc->m_NpcState == NPC_DEAD || pNpc->m_iHP == 0)	{
		bResult = FALSE;
		goto fail_return;
	}

	pType = g_pMain.m_Magictype4Array.GetData( magicid );     // Get magic skill table type 4.
	if( !pType ) return;

	//TRACE("magictype4 ,, magicid=%d\n", magicid);

	switch (pType->bBuffType) {	// Depending on which buff-type it is.....
		case 1 :				// HP 올리기..
			break;

		case 2 :				// 방어력 올리기..
			break;

		case 4 :				// 공격력 올리기..
			break;

		case 5 :				// 공격 속도 올리기..
			break;

		case 6 :				// 이동 속도 올리기..
//			if (pNpc->m_MagicType4[pType->bBuffType-1].sDurationTime > 0) {
//				result = 0 ;
//				goto fail_return ;					
//			}
//			else {
				pNpc->m_MagicType4[pType->bBuffType-1].byAmount = pType->bSpeed;
				pNpc->m_MagicType4[pType->bBuffType-1].sDurationTime = pType->sDuration;
				pNpc->m_MagicType4[pType->bBuffType-1].tStartTime = UNIXTIME;
				pNpc->m_fSpeed_1 = (float)(pNpc->m_fOldSpeed_1 * ((double)pType->bSpeed / 100));
				pNpc->m_fSpeed_2 = (float)(pNpc->m_fOldSpeed_2 * ((double)pType->bSpeed / 100));
				//TRACE("executeType4 ,, speed1=%.2f, %.2f,, type=%d, cur=%.2f, %.2f\n", pNpc->m_fOldSpeed_1, pNpc->m_fOldSpeed_2, pType->bSpeed, pNpc->m_fSpeed_1, pNpc->m_fSpeed_2);
//			}
			break;

		case 7 :				// 능력치 올리기...
			break;

		case 8 :				// 저항력 올리기...
			break;

		case 9 :				// 공격 성공율 및 회피 성공율 올리기..
			break;	

		default :
			bResult = FALSE;
			goto fail_return;
	}

	result	<< uint8(MAGIC_EFFECTING) << magicid << uint16(sid) << uint16(tid)
			<< uint16(data1) << uint16(bResult) << uint16(data3)
			<< uint16(0) << uint16(0) << uint16(0);
	g_pMain.Send(&result);
	return;

fail_return:
	result	<< uint8(MAGIC_FAIL) << magicid << uint16(sid) << uint16(tid)
			<< uint16(0) << uint16(0) << uint16(0)
			<< uint16(0) << uint16(0) << uint16(0);
	g_pMain.Send(&result);
}

void CMagicProcess::ExecuteType5(int magicid)
{
	return;
}

void CMagicProcess::ExecuteType6(int magicid)
{
	return;
}

void CMagicProcess::ExecuteType7(int magicid)
{
	return;
}

void CMagicProcess::ExecuteType8(int magicid)
{
	return;
}

void CMagicProcess::ExecuteType9(int magicid)
{
	return;
}

short CMagicProcess::GetMagicDamage(int tid, int total_hit, int attribute, int dexpoint, int righthand_damage)
{
	short damage = 0, temp_hit = 0 ; 
	int random = 0, total_r = 0 ;
	BYTE result;
	BOOL bSign = TRUE;			// FALSE이면 -, TRUE이면 +

	if( tid < NPC_BAND || tid > INVALID_BAND) return 0;     // Check if target id is valid.

	CNpc* pNpc = NULL;              
	pNpc = g_pMain.m_arNpc.GetData(tid-NPC_BAND);
	if(pNpc == NULL || pNpc->m_NpcState == NPC_DEAD || pNpc->m_iHP == 0)	return 0;
	if(pNpc->m_proto->m_tNpcType == NPC_ARTIFACT||pNpc->m_proto->m_tNpcType == NPC_PHOENIX_GATE || pNpc->m_proto->m_tNpcType == NPC_GATE_LEVER || pNpc->m_proto->m_tNpcType == NPC_SPECIAL_GATE ) return 0;
	
	//result = m_pSrcUser->GetHitRate(m_pSrcUser->m_fHitrate / pNpc->m_sEvadeRate ); 
	result = SUCCESS;
		
	if (result != FAIL) {		// In case of SUCCESS (and SUCCESS only!) .... 
		switch (attribute) {
			case FIRE_R	:
				total_r = pNpc->m_byFireR;
				break;
			case COLD_R :
				total_r = pNpc->m_byColdR;
				break;
			case LIGHTNING_R :
				total_r = pNpc->m_byLightningR ;
				break;
			case MAGIC_R :
				total_r = pNpc->m_byMagicR ;
				break;
			case DISEASE_R :
				total_r = pNpc->m_byDiseaseR ;
				break;
			case POISON_R :			
				total_r = pNpc->m_byPoisonR ;
				break;
		}	

		total_hit = (total_hit * (dexpoint  + 20)) / 170;

		if(total_hit < 0)	{
			total_hit = abs(total_hit);
			bSign = FALSE;
		}
		
		damage = (short)(total_hit - (0.7f * total_hit * total_r / 200)) ;
		random = myrand (0, damage) ;
		damage = (short)((0.7f * (total_hit - (0.9f * total_hit * total_r / 200))) + 0.2f * random);
//		damage = damage + (3 * righthand_damage);
		damage = damage + righthand_damage;
	}
	else damage = 0 ;

	if(bSign == FALSE && damage != 0)	{
		damage = - damage;
	}

	// Apply boost for skills matching weather type.
	// This isn't actually used officially, but I think it's neat...
	GetWeatherDamage(damage, attribute);
		
	//return 1;
	return damage;
}

short CMagicProcess::AreaAttack(int magictype, int magicid, int moral, int data1, int data2, int data3, int dexpoint, int righthand_damage)
{
	_MAGIC_TYPE3* pType3 = NULL;
	_MAGIC_TYPE4* pType4 = NULL;
	int radius = 0; 

	if(magictype == 3)	{
		pType3 = g_pMain.m_Magictype3Array.GetData( magicid );      // Get magic skill table type 3.
		if( !pType3 )	{
			TRACE("#### CMagicProcess-AreaAttack Fail : magic table3 error ,, magicid=%d\n", magicid);
			return 0;
		}
		radius = pType3->bRadius;
	}
	else if(magictype == 4)	{
		pType4 = g_pMain.m_Magictype4Array.GetData( magicid );      // Get magic skill table type 3.
		if( !pType4 )	{
			TRACE("#### CMagicProcess-AreaAttack Fail : magic table4 error ,, magicid=%d\n", magicid);
			return 0;
		}
		radius = pType4->bRadius;
	}

	if( radius <= 0 )	{
		TRACE("#### CMagicProcess-AreaAttack Fail : magicid=%d, radius = %d\n", magicid, radius);
		return 0;
	}

	int region_x = data1 / VIEW_DIST;
	int region_z = data3 / VIEW_DIST;

	MAP* pMap = m_pSrcUser->GetMap();
	if(pMap == NULL) return 0;

	int min_x = region_x - 1;	if(min_x < 0) min_x = 0;
	int min_z = region_z - 1;	if(min_z < 0) min_z = 0;
	int max_x = region_x + 1;	if(max_x > pMap->GetXRegionMax()) max_x = pMap->GetXRegionMax();
	int max_z = region_z + 1;	if(min_z > pMap->GetZRegionMax()) min_z = pMap->GetZRegionMax();

	int search_x = max_x - min_x + 1;		
	int search_z = max_z - min_z + 1;	
	
	int i, j;

	for(i = 0; i < search_x; i++)	{
		for(j = 0; j < search_z; j++)	{
			AreaAttackDamage(magictype, min_x+i, min_z+j, magicid, moral, data1, data2, data3, dexpoint, righthand_damage);
		}
	}

	//damage = GetMagicDamage(tid, pType->sFirstDamage, pType->bAttribute);

	return 1;
}

void CMagicProcess::AreaAttackDamage(int magictype, int rx, int rz, int magicid, int moral, int data1, int data2, int data3, int dexpoint, int righthand_damage)
{
	MAP* pMap = m_pSrcUser->GetMap();
	if (pMap == NULL) return;
	// 자신의 region에 있는 UserArray을 먼저 검색하여,, 가까운 거리에 유저가 있는지를 판단..
	if(rx < 0 || rz < 0 || rx > pMap->GetXRegionMax() || rz > pMap->GetZRegionMax())	{
		TRACE("#### CMagicProcess-AreaAttackDamage() Fail : [nid=%d, name=%s], nRX=%d, nRZ=%d #####\n", m_pSrcUser->m_iUserId, m_pSrcUser->m_strUserID, rx, rz);
		return;
	}

	_MAGIC_TYPE3* pType3 = NULL;
	_MAGIC_TYPE4* pType4 = NULL;
	_MAGIC_TABLE* pMagic = NULL;

	int damage = 0, tid = 0, target_damage = 0, attribute = 0;
	float fRadius = 0; 

	pMagic = g_pMain.m_MagictableArray.GetData( magicid );   // Get main magic table.
	if( !pMagic )	{
		TRACE("#### CMagicProcess-AreaAttackDamage Fail : magic maintable error ,, magicid=%d\n", magicid);
		return;
	}

	if(magictype == 3)	{
		pType3 = g_pMain.m_Magictype3Array.GetData( magicid );      // Get magic skill table type 3.
		if( !pType3 )	{
			TRACE("#### CMagicProcess-AreaAttackDamage Fail : magic table3 error ,, magicid=%d\n", magicid);
			return;
		}
		target_damage = pType3->sFirstDamage;
		attribute = pType3->bAttribute;
		fRadius = (float)pType3->bRadius;
	}
	else if(magictype == 4)	{
		pType4 = g_pMain.m_Magictype4Array.GetData( magicid );      // Get magic skill table type 3.
		if( !pType4 )	{
			TRACE("#### CMagicProcess-AreaAttackDamage Fail : magic table4 error ,, magicid=%d\n", magicid);
			return;
		}
		fRadius = (float)pType4->bRadius;
	}

	if( fRadius <= 0 )	{
		TRACE("#### CMagicProcess-AreaAttackDamage Fail : magicid=%d, radius = %d\n", magicid, fRadius);
		return;
	}


	__Vector3 vStart, vEnd;
	CNpc* pNpc = NULL ;      // Pointer initialization!
	float fDis = 0.0f;
	vStart.Set((float)data1, (float)0, (float)data3);
	int count = 0, total_mon = 0, attack_type=0;
	int* pNpcIDList = NULL;
	BOOL bResult = TRUE;

	EnterCriticalSection( &g_region_critical );

	CRegion *pRegion = &pMap->m_ppRegion[rx][rz];
	total_mon = pRegion->m_RegionNpcArray.GetSize();
	pNpcIDList = new int[total_mon];

	foreach_stlmap (itr, pRegion->m_RegionNpcArray)
		pNpcIDList[count++] = *itr->second;

	LeaveCriticalSection(&g_region_critical);

	for(int i = 0; i < total_mon; i++)
	{
		int nid = pNpcIDList[i];
		if( nid < NPC_BAND ) continue;
		pNpc = (CNpc*)g_pMain.m_arNpc.GetData(nid - NPC_BAND);

		if (pNpc == NULL || pNpc->m_NpcState == NPC_DEAD)
			continue;

		if( m_pSrcUser->m_bNation == pNpc->m_byGroup ) continue;
		vEnd.Set(pNpc->m_fCurX, pNpc->m_fCurY, pNpc->m_fCurZ); 
		fDis = pNpc->GetDistance(vStart, vEnd);

		if(fDis > fRadius)
			continue;

		if(magictype == 3)	{	// 타잎 3일 경우...
			damage = GetMagicDamage(pNpc->m_sNid+NPC_BAND, target_damage, attribute, dexpoint, righthand_damage);
			TRACE("Area magictype3 ,, magicid=%d, damage=%d\n", magicid, damage);
			if(damage >= 0)	{
				bResult = pNpc->SetHMagicDamage(damage);
			}
			else	{
				damage = abs(damage);
				if(pType3->bAttribute == 3)   attack_type = 3; // 기절시키는 마법이라면.....
				else attack_type = magicid;

				if(pNpc->SetDamage(attack_type, damage, m_pSrcUser->m_strUserID, m_pSrcUser->m_iUserId + USER_BAND) == FALSE)	{
					// Npc가 죽은 경우,,
					pNpc->SendExpToUserList(); // 경험치 분배!!
					pNpc->SendDead();
					m_pSrcUser->SendAttackSuccess(pNpc->m_sNid+NPC_BAND, MAGIC_ATTACK_TARGET_DEAD, damage, pNpc->m_iHP);
				}
				else	{
					m_pSrcUser->SendAttackSuccess(pNpc->m_sNid+NPC_BAND, ATTACK_SUCCESS, damage, pNpc->m_iHP);
				}
			}

			// 패킷 전송.....
			//if ( pMagic->bType[1] == 0 || pMagic->bType[1] == 3 ) 
			{
				Packet result(AG_MAGIC_ATTACK_RESULT, uint8(MAGIC_EFFECTING));
				result	<< magicid << m_pSrcUser->m_iUserId << uint16(pNpc->m_sNid+NPC_BAND)
						<< uint16(data1) << uint16(bResult) << uint16(data3)
						<< uint16(moral) << uint16(0) << uint16(0);
				g_pMain.Send(&result);
			}
		}
		else if(magictype == 4)	{	// 타잎 4일 경우...
			bResult = TRUE;
			switch (pType4->bBuffType) {	// Depending on which buff-type it is.....
				case 1 :				// HP 올리기..
					break;

				case 2 :				// 방어력 올리기..
					break;

				case 4 :				// 공격력 올리기..
					break;

				case 5 :				// 공격 속도 올리기..
					break;

				case 6 :				// 이동 속도 올리기..
					//if (pNpc->m_MagicType4[pType4->bBuffType-1].sDurationTime > 0) {
					//	result = 0 ;
					//}
					//else {
						pNpc->m_MagicType4[pType4->bBuffType-1].byAmount = pType4->bSpeed;
						pNpc->m_MagicType4[pType4->bBuffType-1].sDurationTime = pType4->sDuration;
						pNpc->m_MagicType4[pType4->bBuffType-1].tStartTime = UNIXTIME;
						pNpc->m_fSpeed_1 = (float)(pNpc->m_fOldSpeed_1 * ((double)pType4->bSpeed / 100));
						pNpc->m_fSpeed_2 = (float)(pNpc->m_fOldSpeed_2 * ((double)pType4->bSpeed / 100));
					//}
					break;

				case 7 :				// 능력치 올리기...
					break;

				case 8 :				// 저항력 올리기...
					break;

				case 9 :				// 공격 성공율 및 회피 성공율 올리기..
					break;	

				default :
					bResult = FALSE;
					break;
			}

			TRACE("Area magictype4 ,, magicid=%d\n", magicid);

			Packet result(AG_MAGIC_ATTACK_RESULT, uint8(MAGIC_EFFECTING));
			result	<< magicid << m_pSrcUser->m_iUserId << uint16(pNpc->m_sNid+NPC_BAND)
					<< uint16(data1) << uint16(bResult) << uint16(data3)
					<< uint16(0) << uint16(0) << uint16(0);
			g_pMain.Send(&result);
		}
	}

	if(pNpcIDList)	{
		delete [] pNpcIDList;
		pNpcIDList = NULL;
	}
}

short CMagicProcess::GetWeatherDamage(short damage, short attribute)
{
	int iWeather = g_pMain.m_iWeather;
	if ((iWeather == WEATHER_FINE && attribute == ATTRIBUTE_FIRE)
		|| (iWeather == WEATHER_RAIN && attribute == ATTRIBUTE_LIGHTNING)
		|| (iWeather == WEATHER_SNOW && attribute == ATTRIBUTE_ICE))
		damage = (damage * 110) / 100; // 10% damage output increase

	return damage;
}
