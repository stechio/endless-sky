/* ShipInfoDisplay.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ShipInfoDisplay.h"

#include "Color.h"
#include "Depreciation.h"
#include "FillShader.h"
#include "Format.h"
#include "GameData.h"
#include "Outfit.h"
#include "Ship.h"
#include "Table.h"

#include <algorithm>
#include <map>
#include <sstream>

using namespace std;



ShipInfoDisplay::ShipInfoDisplay(const Ship &ship, const Depreciation &depreciation, int day)
{
	Update(ship, depreciation, day);
}



// Call this every time the ship changes.
void ShipInfoDisplay::Update(const Ship &ship, const Depreciation &depreciation, int day)
{
	UpdateDescription(ship.Description(), ship.Attributes().Licenses(), true);
	UpdateAttributes(ship, depreciation, day);
	UpdateOutfits(ship, depreciation, day);
	
	maximumHeight = max(descriptionHeight, max(attributesHeight, outfitsHeight));
}



int ShipInfoDisplay::GetAttributesHeight(bool sale) const
{
	return attributesHeight + (sale ? saleHeight : 0);
}



int ShipInfoDisplay::OutfitsHeight() const
{
	return outfitsHeight;
}



void ShipInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	DrawAttributes(topLeft, false);
}



// Draw each of the panels.
void ShipInfoDisplay::DrawAttributes(const Point &topLeft, const bool sale) const
{
	// Header.
	Point point = Draw(topLeft, attributeHeaderLabels, attributeHeaderValues);

	// Sale info.
	if(sale)
	{
		point = Draw(point, saleLabels, saleValues);

		const Color &color = *GameData::Colors().Get("medium");
		FillShader::Fill(point + Point(.5 * WIDTH, 5.), Point(WIDTH - 20., 1.), color);
	}
	else
	{
		point -= Point(0, 10.);
	}

	// Body.
	point = Draw(point, attributeLabels, attributeValues);
}



void ShipInfoDisplay::DrawOutfits(const Point &topLeft) const
{
	Draw(topLeft, outfitLabels, outfitValues);
}



void ShipInfoDisplay::UpdateAttributes(const Ship &ship, const Depreciation &depreciation, int day)
{
	bool isGeneric = ship.Name().empty() || ship.GetPlanet();
	
	attributeHeaderLabels.clear();
	attributeHeaderValues.clear();

	attributeHeaderLabels.push_back("Model");
	attributeHeaderValues.push_back(ship.ModelName());
	attributesHeight = 20;

	attributeLabels.clear();
	attributeValues.clear();
	
	const Outfit &attributes = ship.Attributes();
	
	int64_t fullCost = ship.Cost();
	int64_t depreciated = depreciation.Value(ship, day);
	if(depreciated == fullCost)
		attributeLabels.push_back("Cost");
	else
	{
		ostringstream out;
		out << "Cost (" << (100 * depreciated) / fullCost << "%)";
		attributeLabels.push_back(out.str());
	}
	attributeValues.push_back(Format::Credits(depreciated));
	attributesHeight += 20;
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;

	if(attributes.Get("shield generation"))
	{
		attributeLabels.push_back("Shields (charge)");
		attributeValues.push_back(Format::Number(attributes.Get("shields"))
				+ " (" + Format::Number(60. * attributes.Get("shield generation")) + "/s)");
	}
	else
	{
		attributeLabels.push_back("Shields");
		attributeValues.push_back(Format::Number(attributes.Get("shields")));
	}
	attributesHeight += 20;

	if(attributes.Get("hull repair rate"))
	{
		attributeLabels.push_back("Hull (repair)");
		attributeValues.push_back(Format::Number(attributes.Get("hull"))
			+ " (" + Format::Number(60. * attributes.Get("hull repair rate")) + "/s)");
	}
	else
	{
		attributeLabels.push_back("Hull");
		attributeValues.push_back(Format::Number(attributes.Get("hull")));
	}
	attributesHeight += 20;

	double emptyMass = ship.Mass();
	attributeLabels.push_back("Displacement");
	attributeValues.push_back(Format::Number(emptyMass) + " tons");
	attributesHeight += 20;

	attributeLabels.push_back(isGeneric ? "Deadweight" : "Deadweight (free)");
	if(isGeneric)
		attributeValues.push_back(Format::Number(attributes.Get("cargo space")) + " tons");
	else
		attributeValues.push_back(Format::Number(attributes.Get("cargo space"))
			+ " (" + Format::Number(attributes.Get("cargo space") - ship.Cargo().Used()) + ") tons");
	attributesHeight += 20;

	if(ship.RequiredCrew() != attributes.Get("bunks"))
	{
		attributeLabels.push_back("Crew (min - max)");
		attributeValues.push_back(Format::Number(ship.RequiredCrew()) + " - " + Format::Number(attributes.Get("bunks")));
	}
	else
	{
		attributeLabels.push_back("Crew");
		attributeValues.push_back(ship.RequiredCrew() == 0 ? "(unmanned)" : Format::Number(ship.RequiredCrew()));
	}
	attributesHeight += 20;

	attributeLabels.push_back(isGeneric ? "Fuel capacity" : "Fuel");
	double fuelCapacity = attributes.Get("fuel capacity");
	if(isGeneric)
		attributeValues.push_back(Format::Number(fuelCapacity));
	else
		attributeValues.push_back(Format::Number(ship.Fuel() * fuelCapacity)
			+ " / " + Format::Number(fuelCapacity));
	attributesHeight += 20;
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	attributeLabels.push_back("Performance");
	attributeValues.push_back(string());
	attributesHeight += 20;

	double fullMass = emptyMass + (isGeneric ? attributes.Get("cargo space") : ship.Cargo().Used());
	isGeneric &= (fullMass != emptyMass);
	double forwardThrust = attributes.Get("thrust") ? attributes.Get("thrust") : attributes.Get("afterburner thrust");

	attributeLabels.push_back("Speed");
	attributeValues.push_back(Format::Number(60. * 50 * forwardThrust / attributes.Get("drag")) + " km/h");
	attributesHeight += 20;

	attributeLabels.push_back("Acceleration");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(3600. / 36 * forwardThrust / fullMass));
	else
		attributeValues.push_back(Format::Number(3600. / 36 * forwardThrust / fullMass)
			+ " - " + Format::Number(3600. / 36 * forwardThrust / emptyMass));
	attributesHeight += 20;
	
	attributeLabels.push_back("Turning");
	if(!isGeneric)
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / fullMass / 360));
	else
		attributeValues.push_back(Format::Number(60. * attributes.Get("turn") / fullMass / 360)
			+ " - " + Format::Number(60. * attributes.Get("turn") / emptyMass  / 360));
	attributesHeight += 20;
	
	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	attributeLabels.push_back("Chassis");
	attributeValues.push_back(string());
	attributesHeight += 20;

	// Find out how much outfit, engine, and weapon space the chassis has.
	map<string, double> chassis;
	static const vector<string> NAMES = {
		"Outfit space", "outfit space",
		"    Weapon capacity", "weapon capacity",
		"    Engine capacity", "engine capacity",
		"Gun ports", "gun ports",
		"Turret mounts", "turret mounts"
	};
	for(unsigned i = 1; i < NAMES.size(); i += 2)
		chassis[NAMES[i]] = attributes.Get(NAMES[i]);
	for(const auto &it : ship.Outfits())
		for(auto &cit : chassis)
			cit.second -= it.second * it.first->Get(cit.first);
	for(unsigned i = 0; i < NAMES.size(); i += 2)
	{
		attributeLabels.push_back(NAMES[i]);
		attributeValues.push_back(Format::Number(chassis[NAMES[i + 1]])
			+ " (" + Format::Number(attributes.Get(NAMES[i + 1])) + ")");
		attributesHeight += 20;
	}
	
	if(ship.BaysFree(false))
	{
		attributeLabels.push_back("Drone bays");
		attributeValues.push_back(to_string(ship.BaysFree(false)));
		attributesHeight += 20;
	}
	if(ship.BaysFree(true))
	{
		attributeLabels.push_back("Fighter bays");
		attributeValues.push_back(to_string(ship.BaysFree(true)));
		attributesHeight += 20;
	}

	attributeLabels.push_back(string());
	attributeValues.push_back(string());
	attributesHeight += 10;
	attributeLabels.push_back("Energy");
	attributeValues.push_back(string());
	attributesHeight += 20;

	attributeLabels.push_back("Batteries");
	attributeValues.push_back(Format::Integer(attributes.Get("energy capacity")));
	attributesHeight += 20;

	attributeLabels.push_back("Generators ");
	attributeValues.push_back(Format::Integer(
			60. * (attributes.Get("energy generation")
				+ attributes.Get("solar collection")
				+ attributes.Get("fuel energy")
				- attributes.Get("energy consumption"))));
	attributesHeight += 20;

	attributeLabels.push_back("Engines ");
	attributeValues.push_back(Format::Integer(
		-60. * (max(attributes.Get("thrusting energy"), attributes.Get("reverse thrusting energy"))
			+ attributes.Get("turning energy")
			+ attributes.Get("afterburner energy"))));
	attributesHeight += 20;

	double firingEnergy = 0.;
	for(const auto &it : ship.Outfits())
		if(it.first->IsWeapon() && it.first->Reload())
		{
			firingEnergy += it.second * it.first->FiringEnergy() / it.first->Reload();
		}
	attributeLabels.push_back("Weapons ");
	attributeValues.push_back(Format::Integer(-60. * firingEnergy));
	attributesHeight += 20;

	double shieldEnergy = attributes.Get("shield energy");
	double hullEnergy = attributes.Get("hull energy");
	if(shieldEnergy || hullEnergy) {
		attributeLabels.push_back((shieldEnergy && hullEnergy) ? "Shields + Hull" :
			hullEnergy ? "Repairing hull" : "Charging shields");
		attributeValues.push_back(Format::Integer(-60. * (shieldEnergy + hullEnergy)));
		// Pad by 10 pixels on the top and bottom.
		attributesHeight += 20;
	}
	attributesHeight += 30;
}



void ShipInfoDisplay::UpdateOutfits(const Ship &ship, const Depreciation &depreciation, int day)
{
	outfitLabels.clear();
	outfitValues.clear();
	outfitsHeight = 20;
	
	map<string, map<string, int>> listing;
	for(const auto &it : ship.Outfits())
		listing[it.first->Category()][it.first->Name()] += it.second;
	
	for(const auto &cit : listing)
	{
		// Pad by 10 pixels before each category.
		if(&cit != &*listing.begin())
		{
			outfitLabels.push_back(string());
			outfitValues.push_back(string());
			outfitsHeight += 10;
		}
				
		outfitLabels.push_back(cit.first);
		outfitValues.push_back(string());
		outfitsHeight += 20;
		
		for(const auto &it : cit.second)
		{
			outfitLabels.push_back(it.first);
			outfitValues.push_back(to_string(it.second));
			outfitsHeight += 20;
		}
	}
	
	
	int64_t totalCost = depreciation.Value(ship, day);
	int64_t chassisCost = depreciation.Value(GameData::Ships().Get(ship.ModelName()), day);
	saleLabels.clear();
	saleValues.clear();
	saleHeight = 20;
	
	saleLabels.push_back("This ship will sell for:");
	saleValues.push_back(string());
	saleHeight += 20;
	saleLabels.push_back("empty hull:");
	saleValues.push_back(Format::Credits(chassisCost));
	saleHeight += 20;
	saleLabels.push_back("  + outfits:");
	saleValues.push_back(Format::Credits(totalCost - chassisCost));
	saleHeight += 5;
}
