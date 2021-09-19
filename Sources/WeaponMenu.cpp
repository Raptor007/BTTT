/*
 *  WeaponMenu.cpp
 */

#include "WeaponMenu.h"

#include "BattleTechGame.h"
#include "Num.h"
#include "HexBoard.h"


WeaponMenu::WeaponMenu( HexBoard *container )
{
	Name = "WeaponMenu";
	
	Selected = NULL;
	
	Rect.w = 640;
	Rect.x = Raptor::Game->Gfx.W/2 - Rect.w/2;
	Rect.y = 10;
	Rect.h = 200;
	
	Red = 0.4f;
	Green = 0.4f;
	Blue = 0.4f;
	Alpha = 0.75f;
	
	ItemFont = Raptor::Game->Res.GetFont( "ProFont.ttf", 18 );
	
	Container = container;
	Update();
}


WeaponMenu::~WeaponMenu()
{
}


void WeaponMenu::Draw( void )
{
	if( ! Update( false ) )
		return;
	Window::Draw();
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	if( Selected && Selected->ReadyAndAble() && (Selected->Team == game->TeamTurn) && (Selected->Team == game->MyTeam()) )
	{
		std::string title;
		if( game->State == BattleTech::State::TAG )
			title = "TAG Spotting";
		else if( game->State == BattleTech::State::WEAPON_ATTACK )
			title = "Weapon Attack";
		else if( game->State == BattleTech::State::PHYSICAL_ATTACK )
			title = "Physical Attack";
		ItemFont->DrawText( title, Rect.w/2 + 2, 7, Font::ALIGN_TOP_CENTER, 0,0,0,0.8f );
		ItemFont->DrawText( title, Rect.w/2,     5, Font::ALIGN_TOP_CENTER );
	}
}


bool WeaponMenu::KeyDown( SDLKey key )
{
	if( key == SDLK_ESCAPE )
		Remove();
	else
		return false;
	return true;
}


bool WeaponMenu::MouseDown( Uint8 button )
{
	return true;
}


bool WeaponMenu::Update( bool force )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	Mech *mech = game->SelectedMech();
	if( ! mech )
	{
		Remove();
		return false;
	}
	else if( force || (mech != Selected) )
	{
		Selected = mech;
		
		if( game->State == BattleTech::State::PHYSICAL_ATTACK )
			UpdateMelee();
		else
			UpdateWeapons();
	}
	
	return true;
}


void WeaponMenu::UpdateWeapons( void )
{
	Rect.w = 640;
	Rect.x = Raptor::Game->Gfx.W/2 - Rect.w/2;
	
	RemoveAllElements();
	
	SDL_Rect rect;
	rect.x = 2;
	rect.y = 2;
	rect.w = 16;
	rect.h = 16;
	AddElement( new WeaponMenuCloseButton( &rect, ItemFont ) );
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	HexBoard *hex_board = (HexBoard*) Container;
	Mech *target = game->TargetMech();
	
	rect.h = ItemFont->GetHeight();
	rect.w = rect.h;
	rect.x = 10;
	rect.y = 10 + rect.h;
	
	rect.x += 230;
	AddElement( new Label( &rect, "Loc", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 40;
	AddElement( new Label( &rect, "Ht", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 40;
	AddElement( new Label( &rect, "Dmg", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 80;
	AddElement( new Label( &rect, "Min", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 40;
	AddElement( new Label( &rect, "Sht", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 40;
	AddElement( new Label( &rect, "Med", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 40;
	AddElement( new Label( &rect, "Lng", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x = Rect.w - 60;
	AddElement( new Label( &rect, "Need", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	
	rect.x = 10;
	rect.y += rect.h + 3;
	
	std::set<MechLocation*> arms_firing;
	short longest_range = 0;
	short medium_range = 99;
	short shortest_range = 99;
	short min_range = 0;
	
	for( std::map<uint8_t,uint8_t>::const_iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
	{
		if( weap->first >= Selected->Equipment.size() )
			continue;
		
		MechEquipment *eq = &(Selected->Equipment[ weap->first ]);
		int needed_roll = 99;
		std::map<uint8_t,int8_t>::const_iterator difficulty = hex_board->WeaponsInRange.find( weap->first );
		if( difficulty != hex_board->WeaponsInRange.end() )
			needed_roll = difficulty->second;
		else if( eq->Weapon->TAG == (game->State == BattleTech::State::TAG) )
			needed_roll = Selected->WeaponRollNeeded( target, &(hex_board->Path), eq );
		
		if( eq->Weapon->RangeLong > longest_range )
			longest_range = eq->Weapon->RangeLong;
		if( eq->Weapon->RangeMedium < medium_range )
			medium_range = eq->Weapon->RangeMedium;
		if( eq->Weapon->RangeShort < shortest_range )
			shortest_range = eq->Weapon->RangeShort;
		if( eq->Weapon->RangeMin > min_range )
			min_range = eq->Weapon->RangeMin;
		
		std::string eq_name = eq->Name;
		if( eq->WeaponTC && ! eq->WeaponTC->Damaged )
			eq_name += " + TC";
		if( eq->WeaponFCS && ! eq->WeaponFCS->Damaged )
			eq_name += " + Artemis";
		if( eq->WeaponArcs.count(BattleTech::Arc::REAR) && (eq->WeaponArcs.size() == 1) )
			eq_name += " (Rear)";
		if( eq->Weapon->AmmoPerTon )
			eq_name += std::string("  [") + Num::ToString((int)Selected->TotalAmmo(eq->ID)) + std::string("]");
		AddElement( new Label( &rect, eq_name, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		std::list<Layer*>::iterator line_element = Elements.end();
		line_element --;
		rect.x += 230;
		
		if( eq->Location )
			AddElement( new Label( &rect, eq->Location->ShortName, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 40;
		
		AddElement( new Label( &rect, eq->Weapon->HeatStr, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 40;
		
		AddElement( new Label( &rect, eq->Weapon->DamageStr, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 80;
		
		Label *min = new Label( &rect, eq->Weapon->RangeMin ? Num::ToString(eq->Weapon->RangeMin) : std::string("-"), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( hex_board->Path.Distance <= eq->Weapon->RangeMin )
			min->Blue = 0.5f;
		AddElement( min );
		rect.x += 40;
		
		Label *sht = new Label( &rect, Num::ToString(eq->Weapon->RangeShort), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( (hex_board->Path.Distance > eq->Weapon->RangeMin) && (hex_board->Path.Distance <= eq->Weapon->RangeShort) )
			sht->Red = 0.5f;
		AddElement( sht );
		rect.x += 40;
		
		Label *med = new Label( &rect, Num::ToString(eq->Weapon->RangeMedium), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( (hex_board->Path.Distance > eq->Weapon->RangeShort) && (hex_board->Path.Distance <= eq->Weapon->RangeMedium) )
		{
			med->Red = 0.5f;
			med->Blue = 0.5f;
		}
		AddElement( med );
		rect.x += 40;
		
		Label *lng = new Label( &rect, Num::ToString(eq->Weapon->RangeLong), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( (hex_board->Path.Distance > eq->Weapon->RangeMedium) && (hex_board->Path.Distance <= eq->Weapon->RangeLong) )
			lng->Blue = 0.5f;
		AddElement( lng );
		rect.x = Rect.w - 60;
		
		if( needed_roll <= 12 )
		{
			Label *roll_label = new Label( &rect, Num::ToString(needed_roll), ItemFont, Font::ALIGN_MIDDLE_LEFT );
			AddElement( roll_label );
			if( needed_roll == 12 )
			{
				roll_label->Green = 0.5f;
				roll_label->Blue = 0.5f;
			}
			else if( needed_roll >= 9 )
			{
				roll_label->Green = 0.75f;
				roll_label->Blue = 0.5f;
			}
			else if( needed_roll >= 6 )
				roll_label->Blue = 0.5f;
			else if( needed_roll >= 3 )
			{
				roll_label->Red = 0.5f;
				roll_label->Blue = 0.5f;
			}
			else
				roll_label->Red = 0.5f;
			rect.x += 30;
			
			if( Selected->Team == game->MyTeam() )
			{
				if( eq->Weapon->RapidFire > 1 )
					AddElement( new WeaponMenuDropDown( &rect, ItemFont, Selected, weap->first, weap->second, eq->Weapon->RapidFire ) );
				else
					AddElement( new WeaponMenuCheckBox( &rect, ItemFont, Selected, weap->first, weap->second ) );
			}
			
			if( weap->second && eq->Location && eq->Location->IsArm() )
				arms_firing.insert( eq->Location );
		}
		else
		{
			if( needed_roll < 99 )
			{
				Label *roll_label = new Label( &rect, Num::ToString(needed_roll), ItemFont, Font::ALIGN_MIDDLE_LEFT );
				AddElement( roll_label );
			}
			for( ; line_element != Elements.end(); line_element ++ )
			{
				(*line_element)->Alpha = 0.5f;
				
				if( eq->Damaged )
				{
					(*line_element)->Red = 1.f;
					(*line_element)->Green = 0.f;
					(*line_element)->Blue = 0.f;
				}
				else if( eq->Jammed || (eq->Weapon->AmmoPerTon && ! Selected->FindAmmo(eq->ID)) )
				{
					(*line_element)->Red = 1.f;
					(*line_element)->Green = 0.5f;
					(*line_element)->Blue = 0.f;
				}
			}
		}
		
		rect.x = 10;
		rect.y += rect.h + 3;
	}
	
	if( Selected->Stealth )
	{
		AddElement( new Label( &rect, "Stealth Armor System", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		std::list<Layer*>::iterator line_element = Elements.end();
		line_element --;
		rect.x += 270;
		
		AddElement( new Label( &rect, "10", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		
		if( (game->State != BattleTech::State::WEAPON_ATTACK) || ! Selected->ActiveStealth )
		{
			for( ; line_element != Elements.end(); line_element ++ )
			{
				(*line_element)->Alpha = 0.5f;
				
				if( (! Selected->ECM) || Selected->ECM->Damaged )
				{
					(*line_element)->Red = 1.f;
					(*line_element)->Green = 0.f;
					(*line_element)->Blue = 0.f;
				}
			}
		}
		
		rect.x = 10;
		rect.y += rect.h + 3;
	}
	
	AddElement( new Label( &rect, "Spot for Indirect Fire", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	std::list<Layer*>::iterator line_element = Elements.end();
	line_element --;
	rect.x = Rect.w - 30;
	if( hex_board->Path.LineOfSight && (game->State == BattleTech::State::WEAPON_ATTACK) && ! Selected->FiredTAG() )
	{
		if( Selected->Team == game->MyTeam() )
			AddElement( new WeaponMenuCheckBox( &rect, ItemFont, Selected, 0xFF, Selected->WeaponsToFire[ 0xFF ] ) );
	}
	else if( hex_board->Path.LineOfSight && (game->State == BattleTech::State::TAG) )
		;
	else for( ; line_element != Elements.end(); line_element ++ )
		(*line_element)->Alpha = 0.5f;
	
	rect.x = 10;
	rect.y += rect.h + 8;
	std::string gato = std::string("G:") + Num::ToString(Selected->GunnerySkill) + std::string(" A:") + Num::ToString(Selected->Attack) + std::string(" T:");
	gato += target ? Num::ToString(target->Defense) : std::string("");
	int gato_other = 99;
	if( hex_board->Path.LineOfSight )
	{
		gato_other = Selected->WeaponRollNeeded( target, &(hex_board->Path) );
		if( gato_other < 99 )
			gato_other -= Selected->GunnerySkill + Selected->Attack - (target ? target->Defense : 0);
	}
	else if( target )
		gato_other = target->Spotted;
	gato += std::string(" O:") + ((gato_other < 99) ? Num::ToString(gato_other) : std::string("X"));
	Label *gato_label = new Label( &rect, gato, ItemFont, Font::ALIGN_MIDDLE_LEFT );
	gato_label->Red   = 0.f;
	gato_label->Green = 0.7f;
	AddElement( gato_label );
	
	rect.x = Rect.w - 80;
	Label *range_label = new Label( &rect, "Range " + Num::ToString(hex_board->Path.Distance), ItemFont, Font::ALIGN_MIDDLE_LEFT );
	if( hex_board->Path.Distance > longest_range )
	{
		range_label->Red = 1.f;
		range_label->Green = 0.f;
		range_label->Blue = 0.f;
	}
	else if( hex_board->Path.Distance <= min_range )
	{
		range_label->Red = 1.f;
		range_label->Green = 0.5f;
		range_label->Blue = 0.f;
	}
	else if( hex_board->Path.Distance > medium_range )
	{
		range_label->Red = 1.f;
		range_label->Green = 1.f;
		range_label->Blue = 0.f;
	}
	else if( hex_board->Path.Distance > shortest_range )
	{
		range_label->Red = 0.f;
		range_label->Green = 1.f;
		range_label->Blue = 0.f;
	}
	else
	{
		range_label->Red = 0.f;
		range_label->Green = 1.f;
		range_label->Blue = 1.f;
	}
	AddElement( range_label );
	
	// If prone with two arms intact, one arm is allowed to fire.
	if( Selected->Prone && (arms_firing.size() >= 2) )
	{
		bool need_update = false;
		
		for( std::map<uint8_t,uint8_t>::iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
		{
			if( (weap->first < Selected->Equipment.size()) && (Selected->Equipment[ weap->first ].Location == *(arms_firing.begin())) )
			{
				Selected->WeaponsToFire[ weap->first ] = 0;
				need_update = true;
			}
		}
		
		if( need_update )
		{
			UpdateWeapons();
			return;
		}
	}
	
	if( game->State == BattleTech::State::WEAPON_ATTACK )
	{
		rect.x = 280;
		HeatTotal = new Label( &rect, "", ItemFont, Font::ALIGN_MIDDLE_LEFT ) ;
		AddElement( HeatTotal );
		UpdateHeatTotal();
	}
	else
		HeatTotal = NULL;
	
	Rect.h = rect.y + rect.h + 3;
}


void WeaponMenu::UpdateMelee( void )
{
	Rect.w = 300;
	Rect.x = Raptor::Game->Gfx.W/2 - Rect.w/2;
	
	RemoveAllElements();
	
	SDL_Rect rect;
	rect.x = 2;
	rect.y = 2;
	rect.w = 16;
	rect.h = 16;
	AddElement( new WeaponMenuCloseButton( &rect, ItemFont ) );
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	rect.h = ItemFont->GetHeight();
	rect.w = rect.h;
	rect.x = 10;
	rect.y = 10 + rect.h;
	
	rect.x += 80;
	AddElement( new Label( &rect, "Limb", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 50;
	AddElement( new Label( &rect, "Table", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 60;
	AddElement( new Label( &rect, "Dmg", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x = Rect.w - 60;
	AddElement( new Label( &rect, "Need", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	
	rect.x = 10;
	rect.y += rect.h + 5;
	
	HexBoard *hex_board = (HexBoard*) Container;
	
	uint8_t index = 0;
	for( std::set<MechMelee>::iterator melee = hex_board->PossibleMelee.begin(); melee != hex_board->PossibleMelee.end(); melee ++ )
	{
		std::string name = "Unknown";
		if( melee->Attack == BattleTech::Melee::KICK )
			name = "Kick";
		else if( melee->Attack == BattleTech::Melee::PUNCH )
			name = "Punch";
		else if( melee->Attack == BattleTech::Melee::CLUB )
			name = "Club";
		else if( melee->Attack == BattleTech::Melee::PUSH )
			name = "Push";
		else if( melee->Attack == BattleTech::Melee::HATCHET )
			name = "Hatchet";
		else if( melee->Attack == BattleTech::Melee::SWORD )
			name = "Sword";
		AddElement( new Label( &rect, name, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		std::list<Layer*>::iterator line_element = Elements.end();
		line_element --;
		rect.x += 80;
		
		AddElement( new Label( &rect, Selected->Locations[ melee->Limb ].ShortName, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 50;
		
		std::string table = "Full";
		if( melee->HitTable == BattleTech::HitTable::KICK )
			table = "Kick";
		else if( melee->HitTable == BattleTech::HitTable::PUNCH )
			table = "Punch";
		AddElement( new Label( &rect, table, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 60;
		
		AddElement( new Label( &rect, Num::ToString(melee->Damage), ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x = Rect.w - 60;
		
		if( melee->Difficulty <= 12 )
		{
			Label *roll_label = new Label( &rect, Num::ToString(melee->Difficulty), ItemFont, Font::ALIGN_MIDDLE_LEFT );
			AddElement( roll_label );
			if( melee->Difficulty == 12 )
			{
				roll_label->Green = 0.5f;
				roll_label->Blue = 0.5f;
			}
			else if( melee->Difficulty >= 9 )
			{
				roll_label->Green = 0.75f;
				roll_label->Blue = 0.5f;
			}
			else if( melee->Difficulty >= 6 )
				roll_label->Blue = 0.5f;
			else if( melee->Difficulty >= 3 )
			{
				roll_label->Red = 0.5f;
				roll_label->Blue = 0.5f;
			}
			else
				roll_label->Red = 0.5f;
			rect.x += 30;
			
			if( Selected->Team == game->MyTeam() )
				AddElement( new WeaponMenuCheckBox( &rect, ItemFont, Selected, index, (Selected->SelectedMelee.find( *melee ) != Selected->SelectedMelee.end()) ) );
		}
		else
		{
			for( ; line_element != Elements.end(); line_element ++ )
				(*line_element)->Alpha = 0.5f;
		}
		
		index ++;
		rect.x = 10;
		rect.y += rect.h + 3;
	}
	
	rect.x = 10;
	rect.y += 5;
	std::string pato = std::string("P:") + Num::ToString(Selected->PilotSkill) + std::string(" A:") + Num::ToString(Selected->Attack) + std::string(" T:");
	Mech *target = game->TargetMech();
	pato += target ? Num::ToString(target->Defense) : std::string("");
	int pato_other = hex_board->Path.Modifier;
	if( target && target->Prone )
		pato_other += (hex_board->Path.Distance <= 1) ? -2 : 1;
	if( target && target->Immobile() )
		pato_other -= 4;
	bool ecm = hex_board->Path.TeamECMs.size() > hex_board->Path.TeamECMs.count(Selected->Team);
	int8_t trees = hex_board->Path.PartialCover ? (hex_board->Path.Modifier - 1) : hex_board->Path.Modifier;
	if( trees && (! ecm) && (hex_board->Path.Distance <= Selected->ActiveProbeRange()) )
		pato_other --;
	pato += std::string(" O:") + ((pato_other < 99) ? Num::ToString(pato_other) : std::string("X"));
	Label *pato_label = new Label( &rect, pato, ItemFont, Font::ALIGN_MIDDLE_LEFT );
	pato_label->Red   = 0.f;
	pato_label->Green = 0.7f;
	AddElement( pato_label );
	
	Rect.h = rect.y + rect.h + 3;
}


void WeaponMenu::UpdateHeatTotal( void )
{
	if( ! HeatTotal )
		return;
	
	if( ! Selected )
	{
		HeatTotal->LabelText = "";
		return;
	}
	
	HexBoard *hex_board = (HexBoard*) Container;
	int shot_heat = 0;
	
	for( std::map<uint8_t,uint8_t>::const_iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
	{
		if( weap->first >= Selected->Equipment.size() )
			continue;
		
		int shot_count = Selected->WeaponsToFire[ weap->first ];
		
		std::map<uint8_t,int8_t>::const_iterator difficulty = hex_board->WeaponsInRange.find( weap->first );
		if( (difficulty == hex_board->WeaponsInRange.end()) || (difficulty->second > 12) )
			shot_count = 0;
		
		MechEquipment *eq = &(Selected->Equipment[ weap->first ]);
		shot_heat += shot_count * eq->Weapon->Heat;
	}
	
	if( Selected->ActiveStealth )
		shot_heat += 10;  // Because we display Stealth Armor System and its heat below the weapon list.
	
	int total_heat = shot_heat + Selected->Heat - Selected->HeatDissipation;
	
	HeatTotal->LabelText = Num::ToString(shot_heat)
		+ std::string(" + ") + Num::ToString((int)Selected->Heat)
		+ std::string(" - ") + Num::ToString((int)Selected->HeatDissipation)
		+ std::string(" = ") + Num::ToString(total_heat) + std::string(" Heat");
	
	if( Selected->TSM && (total_heat >= 9) )
		HeatTotal->LabelText += std::string(", TSM");
	
	if( Selected->LifeSupport && Selected->LifeSupport->Damaged && (total_heat >= 15) )
	{
		HeatTotal->Red = 1.f;
		HeatTotal->Green = 0.f;
		HeatTotal->Blue = 1.f;
	}
	else if( total_heat <= 0 )
	{
		HeatTotal->Red = 0.f;
		HeatTotal->Green = 1.f;
		HeatTotal->Blue = 1.f;
	}
	else if( total_heat < 5 )
	{
		HeatTotal->Red = 0.f;
		HeatTotal->Green = 1.f;
		HeatTotal->Blue = 0.f;
	}
	else if( total_heat < 14 )
	{
		HeatTotal->Red = (Selected->TSM && (total_heat >= 9)) ? 0.5f : 1.f;
		HeatTotal->Green = 1.f;
		HeatTotal->Blue = 0.f;
	}
	else if( total_heat < 19 )
	{
		HeatTotal->Red = 1.f;
		HeatTotal->Green = 0.8f;
		HeatTotal->Blue = 0.f;
	}
	else if( total_heat < 30 )
	{
		HeatTotal->Red = 1.f;
		HeatTotal->Green = 0.5f;
		HeatTotal->Blue = 0.f;
	}
	else
	{
		HeatTotal->Red = 1.f;
		HeatTotal->Green = 0.f;
		HeatTotal->Blue = 0.f;
	}
}


void WeaponMenu::SetCount( uint8_t index, uint8_t count )
{
	if( Raptor::Game->State == BattleTech::State::PHYSICAL_ATTACK )
		SetMelee( index, count );
	else
		SetWeapon( index, count );
}


void WeaponMenu::SetWeapon( uint8_t index, uint8_t count )
{
	if( ! Selected )
		return;
	
	Selected->WeaponsToFire[ index ] = count;
	
	bool need_update = false;
	
	if( index == 0xFF )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		HexBoard *hex_board = (HexBoard*) Container;
		hex_board->UpdateWeaponsInRange( Selected, game->TargetMech() );
		need_update = true;
	}
	
	// If prone with two arms intact, one arm is allowed to fire.
	if( Selected->Prone
	&&  (index < Selected->Equipment.size())
	&&  Selected->Equipment[ index ].Location
	&&  Selected->Equipment[ index ].Location->IsArm() )
	{
		for( std::map<uint8_t,uint8_t>::iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
		{
			if( (weap->first < Selected->Equipment.size())
			&&  Selected->Equipment[ weap->first ].Location
			&&  Selected->Equipment[ weap->first ].Location->IsArm()
			&& (Selected->Equipment[ weap->first ].Location != Selected->Equipment[ index ].Location) )
			{
				Selected->WeaponsToFire[ weap->first ] = 0;
				need_update = true;
			}
		}
	}
	
	if( need_update )
		Update();
	else
		UpdateHeatTotal();
}


void WeaponMenu::SetMelee( uint8_t index, uint8_t count )
{
	if( ! Selected )
		return;
	
	HexBoard *hex_board = (HexBoard*) Container;
	
	std::vector<const MechMelee*> possible;
	for( std::set<MechMelee>::const_iterator melee = hex_board->PossibleMelee.begin(); melee != hex_board->PossibleMelee.end(); melee ++ )
		possible.push_back( &*melee );
	
	if( index >= possible.size() )
		return;
	
	bool need_update = false;
	
	if( ! count )
	{
		std::set<MechMelee>::iterator was_chosen = Selected->SelectedMelee.find( *(possible[ index ]) );
		if( was_chosen != Selected->SelectedMelee.end() )
			Selected->SelectedMelee.erase( was_chosen );
	}
	else
	{
		if( Selected->SelectedMelee.size() && ((possible[ index ]->Attack != BattleTech::Melee::PUNCH) || (Selected->SelectedMelee.begin()->Attack != BattleTech::Melee::PUNCH)) )
		{
			Selected->SelectedMelee.clear();
			need_update = true;
		}
		Selected->SelectedMelee.insert( *(possible[ index ]) );
	}
	
	if( need_update )
		Update();
}


// ---------------------------------------------------------------------------


WeaponMenuCloseButton::WeaponMenuCloseButton( SDL_Rect *rect, Font *button_font )
: LabelledButton( rect, button_font, "x", Font::ALIGN_MIDDLE_CENTER, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
}


WeaponMenuCloseButton::~WeaponMenuCloseButton()
{
}


void WeaponMenuCloseButton::Clicked( Uint8 button )
{
	if( button == SDL_BUTTON_LEFT )
		Container->Remove();
}


// ---------------------------------------------------------------------------


WeaponMenuCheckBox::WeaponMenuCheckBox( SDL_Rect *rect, Font *font, Mech *mech, uint8_t equipment_index, bool checked )
: CheckBox( rect, font, "", checked, NULL, NULL, NULL, NULL, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	EquipmentIndex = equipment_index;
}


WeaponMenuCheckBox::~WeaponMenuCheckBox()
{
}


void WeaponMenuCheckBox::Changed( void )
{
	WeaponMenu *weapon_menu = (WeaponMenu*) Container;
	weapon_menu->SetCount( EquipmentIndex, Checked ? 1 : 0 );
}


void WeaponMenuCheckBox::Draw( void )
{
	CheckBox::Draw();
	if( Checked )
		LabelFont->DrawText( "X", Rect.w/2, Rect.h/2, Font::ALIGN_MIDDLE_CENTER );
}


// ---------------------------------------------------------------------------


WeaponMenuDropDown::WeaponMenuDropDown( SDL_Rect *rect, Font *font, Mech *mech, uint8_t equipment_index, uint8_t value, uint8_t maximum )
: DropDown( rect, font, Font::ALIGN_MIDDLE_CENTER, 0, NULL, NULL )
{
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	EquipmentIndex = equipment_index;
	
	for( uint8_t i = 0; i <= maximum; i ++ )
		AddItem( Num::ToString((int)i), Num::ToString((int)i) );
	Value = Num::ToString((int)value);
	Update();
}


WeaponMenuDropDown::~WeaponMenuDropDown()
{
}


void WeaponMenuDropDown::Changed( void )
{
	WeaponMenu *weapon_menu = (WeaponMenu*) Container;
	weapon_menu->SetCount( EquipmentIndex, atoi( Value.c_str() ) );
}
