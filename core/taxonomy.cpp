// SPDX-License-Identifier: GPL-2.0
#include "taxonomy.h"
#include "errorhelper.h"
#include "gettext.h"
#include "pref.h"
#include "subsurface-string.h"
#include "gettextfromc.h"
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <QtGlobal> // for QT_TRANSLATE_NOOP

const char *taxonomy_category_names[TC_NR_CATEGORIES] = {
	QT_TRANSLATE_NOOP("gettextFromC", "None"),
	QT_TRANSLATE_NOOP("gettextFromC", "Ocean"),
	QT_TRANSLATE_NOOP("gettextFromC", "Country"),
	QT_TRANSLATE_NOOP("gettextFromC", "State"),
	QT_TRANSLATE_NOOP("gettextFromC", "County"),
	QT_TRANSLATE_NOOP("gettextFromC", "Town"),
	QT_TRANSLATE_NOOP("gettextFromC", "City"),
	// AI-generated (Claude)
	QT_TRANSLATE_NOOP("gettextFromC", "Dive region"),
	QT_TRANSLATE_NOOP("gettextFromC", "Dive point")
};

// these are the names for geoname.org
const char *taxonomy_api_names[TC_NR_CATEGORIES] = {
	"none",
	"name",
	"countryName",
	"adminName1",
	"adminName2",
	"toponymName",
	"adminName3",
	// AI-generated (Claude): no geonames mapping; user-only fields
	"none",
	"none"
};

std::string taxonomy_get_value(const taxonomy_data &t, enum taxonomy_category cat)
{
	auto it = std::find_if(t.begin(), t.end(), [cat] (const taxonomy &tax) { return tax.category == cat; });
	return it != t.end() ? it->value : std::string();
}

std::string taxonomy_get_country(const taxonomy_data &t)
{
	return taxonomy_get_value(t, TC_COUNTRY);
}

void taxonomy_set_category(taxonomy_data &t, enum taxonomy_category cat, const std::string &value, enum taxonomy_origin origin)
{
	auto it = std::find_if(t.begin(), t.end(), [cat] (const taxonomy &tax) { return tax.category == cat; });
	if (it == t.end()) {
		t.emplace_back();
		it = std::prev(t.end());
	}
	it->value = value;
	it->origin = origin;
	it->category = cat;
}

void taxonomy_set_country(taxonomy_data &t, const std::string &country, enum taxonomy_origin origin)
{
	report_info("%s: set the taxonomy country to %s\n", __func__, country.c_str());
	taxonomy_set_category(t, TC_COUNTRY, country, origin);
}

// AI-generated (Claude)
std::vector<enum taxonomy_category> taxonomy_active_categories()
{
	std::vector<enum taxonomy_category> out;
	out.reserve(GEOCODING_PREF_SLOTS);
	for (int i = 0; i < GEOCODING_PREF_SLOTS; i++) {
		if (prefs.geocoding.category[i] != TC_NONE)
			out.push_back(prefs.geocoding.category[i]);
	}
	return out;
}

std::string taxonomy_get_location_tags(const taxonomy_data &taxonomy, bool for_maintab)
{
	using namespace std::string_literals;
	std::string locationTag;

	if (taxonomy.empty())
		return locationTag;

	/* Check if the user set any of the geocoding categories */
	// AI-generated (Claude): iterate the full prefs slot array
	bool prefs_set = false;
	for (int i = 0; i < GEOCODING_PREF_SLOTS; i++) {
		if (prefs.geocoding.category[i] != TC_NONE)
			prefs_set = true;
	}

	if (!prefs_set && !for_maintab) {
		locationTag = "<small><small>" + gettextFromC::tr("No dive site layout categories set in preferences!").toStdString() +
			      "</small></small>"s;
		return locationTag;
	}
	else if (!prefs_set)
		return locationTag;

	if (for_maintab)
		locationTag = "<small><small>("s + gettextFromC::tr("Tags").toStdString() + ": "s;
	else
		locationTag = "<small><small>"s;
	std::string connector;
	// AI-generated (Claude): iterate the full prefs slot array
	for (int i = 0; i < GEOCODING_PREF_SLOTS; i++) {
		if (prefs.geocoding.category[i] == TC_NONE)
			continue;
		for (auto const &t: taxonomy) {
			if (t.category == prefs.geocoding.category[i]) {
				if (!t.value.empty()) {
					locationTag += connector + t.value;
					connector = " / "s;
				}
				break;
			}
		}
	}

	if (for_maintab)
		locationTag += ")</small></small>"s;
	else
		locationTag += "</small></small>"s;
	return locationTag;
}
