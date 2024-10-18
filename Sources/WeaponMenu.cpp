/*
 *  WeaponMenu.cpp
 */

#include "WeaponMenu.h"

#include "BattleTechGame.h"
#include "Num.h"
#include "HexBoard.h"
#include <algorithm>


WeaponMenu::WeaponMenu( void )
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
	
	Update();
}


WeaponMenu::~WeaponMenu()
{
}


void WeaponMenu::Draw( void )
{
	if( ! Update( false ) )
		return;
	
	Alpha = IsTop() ? 0.75f : 0.66f;
	Window::Draw();
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	if( Selected && Selected->ReadyAndAble() && (Selected->Team == game->TeamTurn) && (Selected->Team == game->MyTeam()) )
	{
		std::string title = game->PhaseName();
		if( game->State == BattleTech::State::TAG )
			title = "TAG Spotting";
		else if( game->State == BattleTech::State::MOVEMENT )
			title = "Movement Phase";
		ItemFont->DrawText( title, Rect.w/2 + 2, 7, Font::ALIGN_TOP_CENTER, 0,0,0,0.8f );
		ItemFont->DrawText( title, Rect.w/2,     5, Font::ALIGN_TOP_CENTER );
	}
}


bool WeaponMenu::KeyDown( SDLKey key )
{
	HexBoard *hex_board = (HexBoard*) Raptor::Game->Layers.Find("HexBoard");
	if( hex_board && (hex_board->Selected == hex_board->MessageInput) )
		return false;
	else if( key == SDLK_ESCAPE )
		RemoveAndUntarget();
	else
		return false;
	return true;
}


bool WeaponMenu::MouseDown( Uint8 button )
{
	MoveToTop();
	Draggable = (button == SDL_BUTTON_LEFT);
	
	if( Window::MouseDown( button ) )
		return true;
	
	// Prevent scrolling from passing through.
	return (button == SDL_BUTTON_WHEELUP) || (button == SDL_BUTTON_WHEELDOWN);
}


bool WeaponMenu::Update( bool force )
{
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	
	Mech *mech = game->SelectedMech();
	if( ! mech )
	{
		RemoveAndUntarget();
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
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	size_t target_count = game->TargetIDs.size();
	bool target_col = ((target_count > 1) && (game->State == BattleTech::State::WEAPON_ATTACK));
	
	if( Rect.w != 640 )
	{
		Rect.w = 640;
		Rect.x = game->Gfx.W/2 - Rect.w/2;
	}
	
	RemoveAllElements();
	
	SDL_Rect rect;
	rect.x = 2;
	rect.y = 2;
	rect.w = 16;
	rect.h = 16;
	AddElement( new WeaponMenuCloseButton( &rect, ItemFont ) );
	
	HexBoard *hex_board = (HexBoard*) game->Layers.Find("HexBoard");
	Mech *target = game->TargetMech();
	
	rect.h = ItemFont->GetHeight();
	rect.w = rect.h;
	rect.x = 10;
	rect.y = 10 + rect.h;
	
	rect.x += 240;
	AddElement( new Label( &rect, "Loc", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 40;
	AddElement( new Label( &rect, "Ht", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 35;
	AddElement( new Label( &rect, "Dmg", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 70;
	AddElement( new Label( &rect, "Min", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 35;
	AddElement( new Label( &rect, "Sht", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 35;
	AddElement( new Label( &rect, "Med", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	rect.x += 35;
	AddElement( new Label( &rect, "Lng", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	if( target_col )
	{
		rect.x += 40;
		AddElement( new Label( &rect, "Tgt", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	}
	rect.x = Rect.w - 60;
	AddElement( new Label( &rect, "Need", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
	
	rect.x = 10;
	rect.y += rect.h + 3;
	
	std::set<MechLocation*> arms_firing;
	short longest_range = 0;
	short medium_range = 99;
	short shortest_range = 99;
	short min_range = 0;
	bool show_checkboxes = (Selected->Team == game->MyTeam()) && ((game->State == BattleTech::State::WEAPON_ATTACK) || (game->State == BattleTech::State::TAG)) && Selected->ReadyAndAble();
	
	for( std::map<uint8_t,uint8_t>::const_iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
	{
		if( weap->first >= Selected->Equipment.size() )
			continue;
		
		MechEquipment *eq = &(Selected->Equipment[ weap->first ]);
		Mech *weap_target = target;
		ShotPath path = hex_board->Paths[0];
		std::map<uint8_t,uint32_t>::iterator target_iter = Selected->WeaponTargets.find( weap->first );
		if( Selected->DeclaredTargets.size() )
		{
			target_iter = Selected->DeclaredTargets.find( weap->first );
			if( target_iter != Selected->DeclaredTargets.end() )
			{
				weap_target = game->GetMech( target_iter->second );
				if( weap_target )
					path = hex_board->PathToTarget( weap_target->ID );
			}
			else
				weap_target = NULL;
		}
		else if( target_iter != Selected->WeaponTargets.end() )
		{
			weap_target = game->GetMech( target_iter->second );
			if( weap_target )
				path = hex_board->PathToTarget( weap_target->ID );
			else
				weap_target = target;
		}
		
		int needed_roll = Selected->WeaponRollNeeded( weap_target, &path, eq, weap_target && (weap_target->ID != game->TargetID) );
		
		if( eq->Weapon->TAG )
		{
			if( (game->State > BattleTech::State::TAG) && (std::find( game->TargetIDs.begin(), game->TargetIDs.end(), Selected->TaggedTarget ) == game->TargetIDs.end()) )
				needed_roll = 99;
		}
		else if( game->State == BattleTech::State::TAG )
			needed_roll = 99;
		
		if( Selected->DeclaredTargets.size() )
		{
			std::map<uint8_t,uint8_t>::const_iterator declared = Selected->DeclaredWeapons.find( weap->first );
			if( (declared == Selected->DeclaredWeapons.end()) || ! declared->second )
				needed_roll = 99;
		}
		
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
		rect.x += 240;
		
		AddElement( new Label( &rect, eq->Location ? eq->Location->ShortName : "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 40;
		
		AddElement( new Label( &rect, eq->Weapon->HeatStr, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		
		AddElement( new Label( &rect, eq->Weapon->DamageStr, ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 70;
		
		Label *min = new Label( &rect, eq->Weapon->RangeMin ? Num::ToString(eq->Weapon->RangeMin) : std::string("-"), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( path.Distance <= eq->Weapon->RangeMin )
			min->Blue = 0.5f;
		AddElement( min );
		rect.x += 35;
		
		Label *sht = new Label( &rect, Num::ToString(eq->Weapon->RangeShort), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( (path.Distance > eq->Weapon->RangeMin) && (path.Distance <= eq->Weapon->RangeShort) )
			sht->Red = 0.5f;
		AddElement( sht );
		rect.x += 35;
		
		Label *med = new Label( &rect, Num::ToString(eq->Weapon->RangeMedium), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( (path.Distance > eq->Weapon->RangeShort) && (path.Distance <= eq->Weapon->RangeMedium) )
		{
			med->Red = 0.5f;
			med->Blue = 0.5f;
		}
		AddElement( med );
		rect.x += 35;
		
		Label *lng = new Label( &rect, Num::ToString(eq->Weapon->RangeLong), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( (path.Distance > eq->Weapon->RangeMedium) && (path.Distance <= eq->Weapon->RangeLong) )
			lng->Blue = 0.5f;
		AddElement( lng );
		
		if( target_col )
		{
			rect.x += 35;
			if( show_checkboxes && ! eq->Weapon->TAG )
			{
				WeaponMenuDropDown *target_dropdown = new WeaponMenuDropDown( &rect, ItemFont, Selected, weap->first, game->TargetIDs, weap_target ? weap_target->ID : 0 );
				target_dropdown->Rect.w = 35;
				AddElement( target_dropdown );
			}
			else if( weap_target && ! eq->Weapon->TAG )
			{
				Label *target_label = new Label( &rect, std::string("#") + Num::ToString(game->TargetNum(weap_target->ID)), ItemFont, Font::ALIGN_MIDDLE_LEFT );
				target_label->Rect.w = 35;
				AddElement( target_label );
			}
			else if( eq->Weapon->TAG && Selected->TaggedTarget )  // FIXME: What if a Mech has multiple TAGs?
			{
				Label *target_label = new Label( &rect, std::string("#") + Num::ToString(game->TargetNum(Selected->TaggedTarget)), ItemFont, Font::ALIGN_MIDDLE_LEFT );
				target_label->Rect.w = 35;
				AddElement( target_label );
			}
		}
		
		std::string needed_str = Num::ToString(needed_roll);
		if( (game->State <= BattleTech::State::MOVEMENT) && weap_target && ! weap_target->TookTurn )
			needed_str += std::string("*");
		
		rect.x = Rect.w - 60;
		if( eq->Weapon->TAG && (game->State > BattleTech::State::TAG) && (std::find( game->TargetIDs.begin(), game->TargetIDs.end(), Selected->TaggedTarget ) != game->TargetIDs.end()) )
			;
		else if( needed_roll <= 12 )
		{
			Label *roll_label = new Label( &rect, needed_str, ItemFont, Font::ALIGN_MIDDLE_LEFT );
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
			else if( needed_roll >= (Raptor::Game->Data.PropertyAsBool("fail_on_2") ? 4 : 3) )
			{
				roll_label->Red = 0.5f;
				roll_label->Blue = 0.5f;
			}
			else
				roll_label->Red = 0.5f;
			rect.x += 30;
			
			if( show_checkboxes )
			{
				if( eq->Weapon->RapidFire > 1 )
					AddElement( new WeaponMenuDropDown( &rect, ItemFont, Selected, weap->first, weap->second, eq->Weapon->RapidFire ) );
				else
					AddElement( new WeaponMenuCheckBox( &rect, ItemFont, Selected, weap->first, weap->second ) );
			}
			// FIXME: Show rapid fire when DeclaredWeapons has accurate fire counts.
			
			if( weap->second && eq->Location && eq->Location->IsArm() )
				arms_firing.insert( eq->Location );
		}
		else
		{
			if( needed_roll < 99 )
			{
				Label *roll_label = new Label( &rect, needed_str, ItemFont, Font::ALIGN_MIDDLE_LEFT );
				AddElement( roll_label );
			}
			for( ; line_element != Elements.end(); line_element ++ )
			{
				if( (*line_element)->Name == "WeaponMenuDropDown" )
					continue;
				
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
	
	// If prone with two arms intact, one arm is allowed to fire.
	if( Selected->Prone && (arms_firing.size() >= 2) )
	{
		bool need_update = false;
		
		for( std::map<uint8_t,uint8_t>::const_iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
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
	
	if( Selected->Stealth )
	{
		AddElement( new Label( &rect, "Stealth Armor System", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		std::list<Layer*>::iterator line_element = Elements.end();
		line_element --;
		
		rect.x += 240;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 40;
		AddElement( new Label( &rect, "10", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 70;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		
		if( target_col )
		{
			rect.x += 50;
			AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		}
		
		if( (game->State != BattleTech::State::WEAPON_ATTACK) || ! Selected->ActiveStealth )
		{
			for( ; line_element != Elements.end(); line_element ++ )
			{
				if( (*line_element)->Name == "WeaponMenuDropDown" )
					continue;
				
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
	
	if( game->State == BattleTech::State::WEAPON_ATTACK )
	{
		AddElement( new Label( &rect, "Spot for Indirect Fire", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		std::list<Layer*>::iterator line_element = Elements.end();
		line_element --;
		
		std::map<uint8_t,uint32_t>::iterator target_iter = Selected->WeaponTargets.find( 0xFF );
		ShotPath path = (target_iter != Selected->WeaponTargets.end()) ? hex_board->PathToTarget( target_iter->second ) : hex_board->Paths[0];
		int8_t spotted = Selected->SpottingModifier( (target_iter != Selected->WeaponTargets.end()) ? game->GetMech(target_iter->second) : target );
		if( Selected->TaggedTarget )
		{
			const Mech *tagged = game->GetMech( Selected->TaggedTarget );
			spotted = tagged ? tagged->Spotted : 99;
			if( std::find( game->TargetIDs.begin(), game->TargetIDs.end(), Selected->TaggedTarget ) == game->TargetIDs.end() )
				spotted = 99;
		}
		else if( Selected->TookTurn && (Selected->DeclaredTargets.find( 0xFF ) == Selected->DeclaredTargets.end()) )
			spotted = 99;
		
		rect.x += 240;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 40;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 70;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		rect.x += 35;
		AddElement( new Label( &rect, "-", ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
		
		if( target_col )
		{
			rect.x += 35;
			if( show_checkboxes && ! Selected->TaggedTarget )
			{
				std::map<uint8_t,uint32_t>::iterator target_iter = Selected->WeaponTargets.find( 0xFF );
				uint32_t target_id = (target_iter != Selected->WeaponTargets.end()) ? target_iter->second : (target ? target->ID : 0);
				WeaponMenuDropDown *target_dropdown = new WeaponMenuDropDown( &rect, ItemFont, Selected, 0xFF, game->TargetIDs, target_id );
				target_dropdown->Rect.w = 35;
				AddElement( target_dropdown );
			}
			else
			{
				std::map<uint8_t,uint32_t>::iterator target_iter = Selected->DeclaredTargets.find( 0xFF );
				uint32_t target_id = (target_iter != Selected->DeclaredTargets.end()) ? target_iter->second : 0;
				if( target_id )
				{
					Label *target_label = new Label( &rect, std::string("#") + Num::ToString(game->TargetNum(target_id)), ItemFont, Font::ALIGN_MIDDLE_LEFT );
					target_label->Rect.w = 35;
					AddElement( target_label );
				}
			}
		}
		
		if( path.LineOfSight && (spotted < 99) )
		{
			rect.x = Rect.w - 60;
			AddElement( new Label( &rect, std::string("+") + Num::ToString(spotted), ItemFont, Font::ALIGN_MIDDLE_LEFT ) );
			
			if( show_checkboxes && ! Selected->TaggedTarget )
			{
				rect.x = Rect.w - 30;
				AddElement( new WeaponMenuCheckBox( &rect, ItemFont, Selected, 0xFF, Selected->WeaponsToFire[ 0xFF ] ) );
			}
		}
		else for( ; line_element != Elements.end(); line_element ++ )
		{
			if( (*line_element)->Name == "WeaponMenuDropDown" )
				continue;
			
			(*line_element)->Alpha = 0.5f;
		}
		
		rect.x = 10;
		rect.y += rect.h + 3;
	}
	
	rect.y += 5;
	
	if( game->State == BattleTech::State::WEAPON_ATTACK )
	{
		rect.x = 290;
		HeatTotal = new Label( &rect, "", ItemFont, Font::ALIGN_MIDDLE_LEFT ) ;
		if( target_count > 1 )
			HeatTotal->Rect.y -= 5;
		AddElement( HeatTotal );
		UpdateHeatTotal();
		
		if( (target_count >= 2) && Selected && Selected->ArmsCanFlip() && ! Selected->TorsoTwist )
		{
			WeaponMenuCheckBox *arm_flip_checkbox = new WeaponMenuCheckBox( &rect, ItemFont, Selected, " Arm Flip", Selected->ArmsFlipped );
			arm_flip_checkbox->Rect.y += rect.h;
			arm_flip_checkbox->Rect.w = 105;
			AddElement( arm_flip_checkbox );
		}
	}
	else
		HeatTotal = NULL;
	
	HexMap *map = game->Map();
	
	int ranges_to_show = target_count ? target_count : 1;
	for( int i = 0; i < ranges_to_show; i ++ )
	{
		// FIXME: Path(s) mess?
		ShotPath path;
		const Hex *hex = NULL;
		if( target_count )
		{
			uint32_t target_id = game->TargetIDs[ i ];
			target = (Mech*) game->Data.GetObject( target_id );
			path = hex_board->PathToTarget( target_id );
		}
		if( hex_board->Paths.size() && ! path.size() )
			path = hex_board->Paths[ 0 ];
		if( path.size() )
		{
			hex = *(path.rbegin());
			if( ! target )
				target = map->MechAt( hex->X, hex->Y );
		}
		
		rect.x = 10;
		if( ranges_to_show > 1 )
		{
			Label *num_label = new Label( &rect, std::string("#") + Num::ToString(i+1), ItemFont, Font::ALIGN_MIDDLE_LEFT );
			AddElement( num_label );
			rect.x += 30;
		}
		std::string gato = std::string("G:") + Num::ToString(Selected->GunnerySkill) + std::string(" A:") + Num::ToString(Selected->Attack) + std::string(" T:");
		gato += (target && (target->TookTurn || (game->State > BattleTech::State::MOVEMENT))) ? Num::ToString(target->Defense) : std::string(" ");
		int gato_other = 99;
		if( path.LineOfSight )
		{
			gato_other = Selected->WeaponRollNeeded( target, &path, NULL, i );
			if( gato_other < 99 )
				gato_other -= Selected->GunnerySkill + Selected->Attack + (target ? target->Defense : 0);
		}
		else if( target )
			gato_other = target->Spotted;
		gato += std::string(" O:") + ((gato_other < 99) ? Num::ToString(gato_other) : std::string(" "));
		Label *gato_label = new Label( &rect, gato, ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( path.LineOfSight )
		{
			gato_label->Red   = 0.2f;
			gato_label->Green = 0.8f;
		}
		else if( gato_other < 99 )  // LRM Indirect Fire
		{
			gato_label->Green = 0.8f;
			gato_label->Blue  = 0.f;
		}
		else
		{
			gato_label->Green = 0.f;
			gato_label->Blue  = 0.f;
		}
		AddElement( gato_label );
		
		rect.x = Rect.w - 80;
		Label *range_label = new Label( &rect, "Range " + Num::ToString(path.Distance), ItemFont, Font::ALIGN_MIDDLE_LEFT );
		if( path.Distance > longest_range )
		{
			range_label->Red = 1.f;
			range_label->Green = 0.f;
			range_label->Blue = 0.f;
		}
		else if( path.Distance <= min_range )
		{
			range_label->Red = 1.f;
			range_label->Green = 0.5f;
			range_label->Blue = 0.f;
		}
		else if( path.Distance > medium_range )
		{
			range_label->Red = 1.f;
			range_label->Green = 1.f;
			range_label->Blue = 0.f;
		}
		else if( path.Distance > shortest_range )
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
		
		if( hex )
		{
			rect.x = Rect.w - 135;
			uint8_t arc = Selected->FiringArc( hex->X, hex->Y );
			std::string arc_name;
			if( arc == BattleTech::Arc::FRONT )
				arc_name = "Front";
			else if( arc == BattleTech::Arc::LEFT_SIDE )
				arc_name = "Left";
			else if( arc == BattleTech::Arc::RIGHT_SIDE )
				arc_name = "Right";
			else if( arc == BattleTech::Arc::REAR )
				arc_name = "Rear";
			Label *arc_label = new Label( &rect, arc_name, ItemFont, Font::ALIGN_MIDDLE_LEFT );
			arc_label->Red   = range_label->Red;
			arc_label->Green = range_label->Green;
			arc_label->Blue  = range_label->Blue;
			AddElement( arc_label );
		}
		
		rect.y += rect.h;
	}
	
	Rect.h = rect.y + 5;
}


void WeaponMenu::UpdateMelee( void )
{
	if( Rect.w != 300 )
	{
		Rect.w = 300;
		Rect.x = Raptor::Game->Gfx.W/2 - Rect.w/2;
	}
	
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
	
	HexBoard *hex_board = (HexBoard*) game->Layers.Find("HexBoard");
	
	Mech *target = game->TargetMech();
	bool show_checkboxes = (! Selected->TookTurn) && (Selected->Team == game->MyTeam());
	
	uint8_t index = 0;
	for( std::set<MechMelee>::const_iterator melee = hex_board->PossibleMelee.begin(); melee != hex_board->PossibleMelee.end(); melee ++ )
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
		
		int8_t difficulty = melee->Difficulty;
		
		if( Selected->TookTurn && target && (Selected->DeclaredTarget == target->ID) )
		{
			bool found = false;
			for( std::set<uint8_t>::const_iterator declared = Selected->DeclaredMelee.begin(); declared != Selected->DeclaredMelee.end(); declared ++ )
			{
				uint8_t attack =  (*declared) & 0x1F;
				uint8_t limb   = ((*declared) & 0xE0) >> 5;
				if( (attack == melee->Attack) && (limb == melee->Limb) )
				{
					found = true;
					break;
				}
			}
			if( ! found )
				difficulty = 99;
		}
		
		if( difficulty <= 12 )
		{
			Label *roll_label = new Label( &rect, Num::ToString(difficulty), ItemFont, Font::ALIGN_MIDDLE_LEFT );
			AddElement( roll_label );
			if( difficulty == 12 )
			{
				roll_label->Green = 0.5f;
				roll_label->Blue = 0.5f;
			}
			else if( difficulty >= 9 )
			{
				roll_label->Green = 0.75f;
				roll_label->Blue = 0.5f;
			}
			else if( difficulty >= 6 )
				roll_label->Blue = 0.5f;
			else if( difficulty >= (Raptor::Game->Data.PropertyAsBool("fail_on_2") ? 4 : 3) )
			{
				roll_label->Red = 0.5f;
				roll_label->Blue = 0.5f;
			}
			else
				roll_label->Red = 0.5f;
			rect.x += 30;
			
			if( show_checkboxes )
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
	pato += target ? Num::ToString(target->Defense) : std::string("");
	int pato_other = hex_board->Paths[0].Modifier; // FIXME: Path(s)
	if( target && target->Prone )
		pato_other += (hex_board->Paths[0].Distance <= 1) ? -2 : 1; // FIXME: Path(s)
	if( target && target->Immobile() )
		pato_other -= 4;
	bool ecm = hex_board->Paths[0].ECMvsTeam( Selected->Team ); // FIXME: Path(s)
	int8_t trees = hex_board->Paths[0].PartialCover ? (hex_board->Paths[0].Modifier - 1) : hex_board->Paths[0].Modifier; // FIXME: Path(s)
	if( trees && (! ecm) && (hex_board->Paths[0].Distance <= Selected->ActiveProbeRange()) ) // FIXME: Path(s)
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
	
	HexBoard *hex_board = (HexBoard*) Raptor::Game->Layers.Find("HexBoard");
	int shot_heat = 0;
	
	for( std::map<uint8_t,uint8_t>::const_iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
	{
		if( weap->first >= Selected->Equipment.size() )
			continue;
		
		int shot_count = Selected->WeaponsToFire[ weap->first ];
		
		std::map<uint8_t,int8_t>::const_iterator difficulty = hex_board->WeaponsInRange.find( weap->first );
		if( (difficulty == hex_board->WeaponsInRange.end()) || (difficulty->second > 12) )
			shot_count = 0;
		
		if( Selected->TookTurn && Selected->DeclaredTargets.size() )
		{
			std::map<uint8_t,uint8_t>::const_iterator declared = Selected->DeclaredWeapons.find( weap->first );
			shot_count = (declared != Selected->DeclaredWeapons.end()) ? declared->second : 0;
		}
		
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
	
	bool was_firing = Selected->FiringWeapons();
	
	if( Selected->WeaponsToFire[ index ] != count )
		Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
	
	Selected->WeaponsToFire[ index ] = count;
	
	// Make sure we update the spotting modifier for firing weapons.
	bool need_update = (was_firing != (bool) Selected->FiringWeapons()) && ! Selected->TaggedTarget;
	
	if( index == 0xFF )
	{
		BattleTechGame *game = (BattleTechGame*) Raptor::Game;
		HexBoard *hex_board = (HexBoard*) game->Layers.Find("HexBoard");
		hex_board->UpdateWeaponsInRange( Selected, game->TargetMech() );
		need_update = true;
	}
	
	// If prone with two arms intact, one arm is allowed to fire.
	if( Selected->Prone
	&&  (index < Selected->Equipment.size())
	&&  Selected->Equipment[ index ].Location
	&&  Selected->Equipment[ index ].Location->IsArm() )
	{
		for( std::map<uint8_t,uint8_t>::const_iterator weap = Selected->WeaponsToFire.begin(); weap != Selected->WeaponsToFire.end(); weap ++ )
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
	
	HexBoard *hex_board = (HexBoard*) Raptor::Game->Layers.Find("HexBoard");
	
	std::vector<const MechMelee*> possible;
	for( std::set<MechMelee>::const_iterator melee = hex_board->PossibleMelee.begin(); melee != hex_board->PossibleMelee.end(); melee ++ )
		possible.push_back( &*melee );
	
	if( index >= possible.size() )
		return;
	
	Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
	
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


void WeaponMenu::SetTarget( uint8_t index, uint32_t target_id )
{
	if( ! Selected )
		return;
	
	if( Selected->WeaponTargets[ index ] != target_id )
	{
		Selected->WeaponTargets[ index ] = target_id;
		
		HexBoard *hex_board = (HexBoard*) Raptor::Game->Layers.Find("HexBoard");
		hex_board->UpdateWeaponsInRange( Selected, ((BattleTechGame*)( Raptor::Game ))->TargetMech() );
		
		Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("i_select.wav") );
		
		Update();
	}
}


void WeaponMenu::SetArmsFlipped( bool arms_flipped )
{
	if( ! Selected )
		return;
	
	if( Selected->ArmsFlipped != arms_flipped )
	{
		Selected->Animate( 0.1, BattleTech::Effect::TORSO_TWIST );
		Selected->ArmsFlipped = arms_flipped;
		
		Raptor::Game->Snd.Play( Raptor::Game->Res.GetSound("m_twist.wav") );
		
		Update();
	}
}


void WeaponMenu::RemoveAndUntarget( void )
{
	Remove();
	
	if( Selected )
		Selected->WeaponTargets.clear();
	
	BattleTechGame *game = (BattleTechGame*) Raptor::Game;
	game->TargetID = 0;
	game->TargetIDs.clear();
	
	HexBoard *hex_board = (HexBoard*) game->Layers.Find("HexBoard");
	if( hex_board )
		hex_board->ClearPath();
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
		((WeaponMenu*) Container)->RemoveAndUntarget();
}


// ---------------------------------------------------------------------------


WeaponMenuCheckBox::WeaponMenuCheckBox( SDL_Rect *rect, Font *font, Mech *mech, uint8_t equipment_index, bool checked )
: CheckBox( rect, font, "", checked, NULL, NULL, NULL, NULL, NULL, NULL )
{
	Red = Green = Blue = 0.f;
	Alpha = 0.75f;
	
	EquipmentIndex = equipment_index;
}


WeaponMenuCheckBox::WeaponMenuCheckBox( SDL_Rect *rect, Font *font, Mech *mech, std::string label, bool checked )
: CheckBox( rect, font, label, checked, NULL, NULL, NULL, NULL, NULL, NULL )
{
	Red = Green = Blue = 0.f;
	Alpha = 0.75f;
	
	EquipmentIndex = 0xFE;  // FIXME: Dirty hack.
}


WeaponMenuCheckBox::~WeaponMenuCheckBox()
{
}


void WeaponMenuCheckBox::Changed( void )
{
	WeaponMenu *weapon_menu = (WeaponMenu*) Container;
	
	if( EquipmentIndex == 0xFE )  // FIXME: Dirty hack.
	{
		weapon_menu->SetArmsFlipped( Checked );
		return;
	}
	
	weapon_menu->SetCount( EquipmentIndex, Checked ? 1 : 0 );
}


void WeaponMenuCheckBox::Draw( void )
{
	CheckBox::Draw();
	//Raptor::Game->Gfx.DrawRect2D( 1, 1, Rect.h - 1, Rect.h - 1, 0, 0.f,0.f,0.f,1.f );
	if( Checked )
		LabelFont->DrawText( "X", (Rect.h+1)/2, (Rect.h+1)/2, Font::ALIGN_MIDDLE_CENTER );
}


// ---------------------------------------------------------------------------


WeaponMenuDropDown::WeaponMenuDropDown( SDL_Rect *rect, Font *font, Mech *mech, uint8_t equipment_index, uint8_t value, uint8_t maximum )
: DropDown( rect, font, Font::ALIGN_MIDDLE_CENTER, 0, NULL, NULL )
{
	Name = "WeaponMenuDropDown";
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	EquipmentIndex = equipment_index;
	TargetSelect = false;
	
	for( uint8_t i = 0; i <= maximum; i ++ )
		AddItem( Num::ToString((int)i), Num::ToString((int)i) );
	Value = Num::ToString((int)value);
	Update();
}


WeaponMenuDropDown::WeaponMenuDropDown( SDL_Rect *rect, Font *font, Mech *mech, uint8_t equipment_index, const std::vector<uint32_t> &target_ids, uint32_t selected )
: DropDown( rect, font, Font::ALIGN_MIDDLE_CENTER, 0, NULL, NULL )
{
	Name = "WeaponMenuDropDown";
	Red = 0.f;
	Green = 0.f;
	Blue = 0.f;
	Alpha = 0.75f;
	
	EquipmentIndex = equipment_index;
	TargetSelect = true;
	
	if( target_ids.size() && ! selected )
		selected = target_ids[ 0 ];
	
	for( uint8_t i = 0; i < target_ids.size(); i ++ )
	{
		uint32_t target_id = target_ids[ i ];
		std::string target_str = Num::ToString((int)target_id);
		AddItem( target_str, std::string("#") + Num::ToString((int)i+1) );
		if( target_id == selected )
			Value = target_str;
	}
	Update();
}


WeaponMenuDropDown::~WeaponMenuDropDown()
{
}


void WeaponMenuDropDown::Changed( void )
{
	WeaponMenu *weapon_menu = (WeaponMenu*) Container;
	
	if( ! TargetSelect )
		weapon_menu->SetCount( EquipmentIndex, atoi( Value.c_str() ) );
	else
		weapon_menu->SetTarget( EquipmentIndex, atoi( Value.c_str() ) );
}
