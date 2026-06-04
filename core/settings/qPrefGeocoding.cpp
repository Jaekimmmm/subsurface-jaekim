// SPDX-License-Identifier: GPL-2.0
#include "qPrefGeocoding.h"
#include "qPrefPrivate.h"

static const QString group = QStringLiteral("geocoding");

qPrefGeocoding *qPrefGeocoding::instance()
{
	static qPrefGeocoding *self = new qPrefGeocoding;
	return self;
}

void qPrefGeocoding::loadSync(bool doSync)
{
	disk_first_taxonomy_category(doSync);
	disk_second_taxonomy_category(doSync);
	disk_third_taxonomy_category(doSync);
	// AI-generated (Claude)
	disk_fourth_taxonomy_category(doSync);
	disk_fifth_taxonomy_category(doSync);
}

void qPrefGeocoding::set_first_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[0]) {
		prefs.geocoding.category[0] = value;
		disk_first_taxonomy_category(true);
		emit instance()->first_taxonomy_categoryChanged(value);
	}
}
void qPrefGeocoding::disk_first_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat0"), prefs.geocoding.category[0], default_prefs.geocoding.category[0]);
	else
		prefs.geocoding.category[0] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat0"), default_prefs.geocoding.category[0]).toInt();
}

void qPrefGeocoding::set_second_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[1]) {
		prefs.geocoding.category[1] = value;
		disk_second_taxonomy_category(true);
		emit instance()->second_taxonomy_categoryChanged(value);
	}
}

void qPrefGeocoding::disk_second_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat1"), prefs.geocoding.category[1], default_prefs.geocoding.category[1]);
	else
		prefs.geocoding.category[1] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat1"), default_prefs.geocoding.category[1]).toInt();
}


void qPrefGeocoding::set_third_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[2]) {
		prefs.geocoding.category[2] = value;
		disk_third_taxonomy_category(true);
		emit instance()->third_taxonomy_categoryChanged(value);
	}
}

void qPrefGeocoding::disk_third_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat2"), prefs.geocoding.category[2], default_prefs.geocoding.category[2]);
	else
		prefs.geocoding.category[2] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat2"), default_prefs.geocoding.category[2]).toInt();
}

// AI-generated (Claude): slots 4 and 5 for the expanded dive-site hierarchy
void qPrefGeocoding::set_fourth_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[3]) {
		prefs.geocoding.category[3] = value;
		disk_fourth_taxonomy_category(true);
		emit instance()->fourth_taxonomy_categoryChanged(value);
	}
}

void qPrefGeocoding::disk_fourth_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat3"), prefs.geocoding.category[3], default_prefs.geocoding.category[3]);
	else
		prefs.geocoding.category[3] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat3"), default_prefs.geocoding.category[3]).toInt();
}

void qPrefGeocoding::set_fifth_taxonomy_category(taxonomy_category value)
{
	if (value != prefs.geocoding.category[4]) {
		prefs.geocoding.category[4] = value;
		disk_fifth_taxonomy_category(true);
		emit instance()->fifth_taxonomy_categoryChanged(value);
	}
}

void qPrefGeocoding::disk_fifth_taxonomy_category(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "cat4"), prefs.geocoding.category[4], default_prefs.geocoding.category[4]);
	else
		prefs.geocoding.category[4] = (enum taxonomy_category)qPrefPrivate::propValue(keyFromGroupAndName(group, "cat4"), default_prefs.geocoding.category[4]).toInt();
}
