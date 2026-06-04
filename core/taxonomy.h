// SPDX-License-Identifier: GPL-2.0
#ifndef TAXONOMY_H
#define TAXONOMY_H

#include <string>
#include <vector>

enum taxonomy_category {
	TC_NONE,
	TC_OCEAN,
	TC_COUNTRY,
	TC_ADMIN_L1,
	TC_ADMIN_L2,
	TC_LOCALNAME,
	TC_ADMIN_L3,
	// AI-generated (Claude): diving-domain categories, user-editable only
	TC_DIVE_REGION,
	TC_DIVE_POINT,
	TC_NR_CATEGORIES
};

enum taxonomy_origin {
	GEOCODED,
	GEOPARSED,
	GEOMANUAL,
	GEOCOPIED
};

extern const char *taxonomy_category_names[TC_NR_CATEGORIES];
extern const char *taxonomy_api_names[TC_NR_CATEGORIES];

struct taxonomy {
	taxonomy_category category = TC_NONE;	/* the category for this tag: ocean, country, admin_l1, admin_l2, localname, etc */
	std::string value;			/* the value returned, parsed, or manually entered for that category */
	taxonomy_origin origin = GEOCODED;
};

/* the data block contains taxonomy structures - unused ones have a tag value of NONE */
using taxonomy_data = std::vector<taxonomy>;

std::string taxonomy_get_value(const taxonomy_data &t, enum taxonomy_category cat);
std::string taxonomy_get_country(const taxonomy_data &t);
std::string taxonomy_get_location_tags(const taxonomy_data &taxonomy, bool for_maintab);
void taxonomy_set_category(taxonomy_data &t, enum taxonomy_category category, const std::string &value, enum taxonomy_origin origin);
void taxonomy_set_country(taxonomy_data &t, const std::string &country, enum taxonomy_origin origin);

// AI-generated (Claude): hierarchy helpers
// Returns the categories enabled in prefs.geocoding.category[], in order,
// skipping TC_NONE slots. This is the user-visible ordering for both the
// dive-site edit form and the tree view.
std::vector<enum taxonomy_category> taxonomy_active_categories();

#endif // TAXONOMY_H
