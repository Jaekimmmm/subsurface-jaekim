// SPDX-License-Identifier: GPL-2.0

#include "command_divesite.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "qt-models/divelocationmodel.h"
#include "qt-models/filtermodels.h"

namespace Command {

// Helper functions to add / remove a set of dive sites

// Add a set of dive sites to the core. The dives that were associated with
// that dive site will be restored to that dive site.
static std::vector<dive_site *> addDiveSites(std::vector<std::unique_ptr<dive_site>> &sites)
{
	std::vector<dive_site *> res;
	QVector<dive *> changedDives;
	res.reserve(sites.size());

	for (std::unique_ptr<dive_site> &ds: sites) {
		// Readd the dives that belonged to this site
		for (dive *d: ds->dives) {
			// TODO: send dive site changed signal
			d->dive_site = ds.get();
			changedDives.push_back(d);
		}

		// Add dive site to core, but remember a non-owning pointer first.
		auto add_res = divelog.sites.register_site(std::move(ds)); // Return ownership to backend.
		res.push_back(add_res.ptr);
		emit diveListNotifier.diveSiteAdded(res.back(), add_res.idx); // Inform frontend of new dive site.
	}

	emit diveListNotifier.divesChanged(changedDives, DiveField::DIVESITE);

	// Clear vector of unused owning pointers
	sites.clear();

	return res;
}

// Remove a set of dive sites. Get owning pointers to them. The dives are set to
// being at no dive site, but the dive site will retain a list of dives, so
// that the dives can be readded to the site on undo.
static std::vector<std::unique_ptr<dive_site>> removeDiveSites(std::vector<dive_site *> &sites)
{
	std::vector<std::unique_ptr<dive_site>> res;
	QVector<dive *> changedDives;
	res.reserve(sites.size());

	for (dive_site *ds: sites) {
		// Reset the dive_site field of the affected dives
		for (dive *d: ds->dives) {
			d->dive_site = nullptr;
			changedDives.push_back(d);
		}

		// Remove dive site from core and take ownership.
		auto pull_res = divelog.sites.pull(ds);
		res.push_back(std::move(pull_res.ptr));
		emit diveListNotifier.diveSiteDeleted(ds, pull_res.idx); // Inform frontend of removed dive site.
	}

	emit diveListNotifier.divesChanged(changedDives, DiveField::DIVESITE);

	sites.clear();

	return res;
}

AddDiveSite::AddDiveSite(const QString &name)
{
	setText(Command::Base::tr("add dive site"));
	sitesToAdd.push_back(std::make_unique<dive_site>());
	sitesToAdd.back()->name = name.toStdString();
}

bool AddDiveSite::workToBeDone()
{
	return true;
}

void AddDiveSite::redo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

void AddDiveSite::undo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

ImportDiveSites::ImportDiveSites(dive_site_table sites, const QString &source)
{
	setText(Command::Base::tr("import dive sites from %1").arg(source));

	for (auto &new_ds: sites) {
		// Don't import dive sites that already exist.
		// We might want to be smarter here and merge dive site data, etc.
		if (divelog.sites.get_same(*new_ds))
			continue;
		sitesToAdd.push_back(std::move(new_ds));
	}
}

bool ImportDiveSites::workToBeDone()
{
	return !sitesToAdd.empty();
}

void ImportDiveSites::redo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

void ImportDiveSites::undo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

DeleteDiveSites::DeleteDiveSites(const QVector<dive_site *> &sites) : sitesToRemove(std::vector<dive_site *>(sites.begin(),sites.end()))
{
	setText(Command::Base::tr("delete %n dive site(s)", "", sites.size()));
}

bool DeleteDiveSites::workToBeDone()
{
	return !sitesToRemove.empty();
}

void DeleteDiveSites::redo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

void DeleteDiveSites::undo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

PurgeUnusedDiveSites::PurgeUnusedDiveSites()
{
	setText(Command::Base::tr("purge unused dive sites"));
	for (const auto &ds: divelog.sites) {
		if (ds->dives.empty())
			sitesToRemove.push_back(ds.get());
	}
}

bool PurgeUnusedDiveSites::workToBeDone()
{
	return !sitesToRemove.empty();
}

void PurgeUnusedDiveSites::redo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

void PurgeUnusedDiveSites::undo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

EditDiveSiteName::EditDiveSiteName(dive_site *dsIn, const QString &name) : ds(dsIn),
	value(name.toStdString())
{
	setText(Command::Base::tr("Edit dive site name"));
}

bool EditDiveSiteName::workToBeDone()
{
	return value != ds->name;
}

void EditDiveSiteName::redo()
{
	swap(ds->name, value);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::NAME); // Inform frontend of changed dive site.
}

void EditDiveSiteName::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteDescription::EditDiveSiteDescription(dive_site *dsIn, const QString &description) : ds(dsIn),
	value(description.toStdString())
{
	setText(Command::Base::tr("Edit dive site description"));
}

bool EditDiveSiteDescription::workToBeDone()
{
	return value != ds->description;
}

void EditDiveSiteDescription::redo()
{
	swap(ds->description, value);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::DESCRIPTION); // Inform frontend of changed dive site.
}

void EditDiveSiteDescription::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteNotes::EditDiveSiteNotes(dive_site *dsIn, const QString &notes) : ds(dsIn),
	value(notes.toStdString())
{
	setText(Command::Base::tr("Edit dive site notes"));
}

bool EditDiveSiteNotes::workToBeDone()
{
	return value != ds->notes;
}

void EditDiveSiteNotes::redo()
{
	swap(ds->notes, value);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::NOTES); // Inform frontend of changed dive site.
}

void EditDiveSiteNotes::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteLocation::EditDiveSiteLocation(dive_site *dsIn, const location_t location) : ds(dsIn),
	value(location)
{
	setText(Command::Base::tr("Edit dive site location"));
}

bool EditDiveSiteLocation::workToBeDone()
{
	bool ok = has_location(&value);
	bool old_ok = has_location(&ds->location);
	if (ok != old_ok)
		return true;
	return ok && value != ds->location;
}

void EditDiveSiteLocation::redo()
{
	std::swap(value, ds->location);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::LOCATION); // Inform frontend of changed dive site.
}

void EditDiveSiteLocation::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteTaxonomy::EditDiveSiteTaxonomy(dive_site *dsIn, taxonomy_data &taxonomy) : ds(dsIn),
	value(taxonomy)
{
	setText(Command::Base::tr("Edit dive site taxonomy"));
}

EditDiveSiteTaxonomy::~EditDiveSiteTaxonomy()
{
}

bool EditDiveSiteTaxonomy::workToBeDone()
{
	// TODO: Apparently we have no way of comparing taxonomies?
	return true;
}

void EditDiveSiteTaxonomy::redo()
{
	std::swap(value, ds->taxonomy);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::TAXONOMY); // Inform frontend of changed dive site.
}

void EditDiveSiteTaxonomy::undo()
{
	// Undo and redo do the same
	redo();
}

MergeDiveSites::MergeDiveSites(dive_site *dsIn, const QVector<dive_site *> &sites) : ds(dsIn)
{
	setText(Command::Base::tr("merge dive sites"));
	sitesToRemove.reserve(sites.size());
	for (dive_site *site: sites) {
		if (site != ds)
			sitesToRemove.push_back(site);
	}
}

bool MergeDiveSites::workToBeDone()
{
	return !sitesToRemove.empty();
}

void MergeDiveSites::redo()
{
	// First, remove all dive sites
	sitesToAdd = removeDiveSites(sitesToRemove);

	// Remember which dives changed so that we can send a single dives-edited signal
	QVector<dive *> divesChanged;

	// The dives of the above dive sites were reset to no dive sites.
	// Add them to the merged-into dive site. Thankfully, we remember
	// the dives in the sitesToAdd vector.
	for (const std::unique_ptr<dive_site> &site: sitesToAdd) {
		for (dive *d: site->dives) {
			ds->add_dive(d);
			divesChanged.push_back(d);
		}
	}
	emit diveListNotifier.divesChanged(divesChanged, DiveField::DIVESITE);
}

void MergeDiveSites::undo()
{
	// Remember which dives changed so that we can send a single dives-edited signal
	QVector<dive *> divesChanged;

	// Before readding the dive sites, unregister the corresponding dives so that they can be
	// readded to their old dive sites.
	for (const std::unique_ptr<dive_site> &site: sitesToAdd) {
		for (dive *d: site->dives) {
			unregister_dive_from_dive_site(d);
			divesChanged.push_back(d);
		}
	}

	sitesToRemove = addDiveSites(sitesToAdd);

	emit diveListNotifier.divesChanged(divesChanged, DiveField::DIVESITE);
}

ApplyGPSFixes::ApplyGPSFixes(const std::vector<DiveAndLocation> &fixes)
{
	setText(Command::Base::tr("apply GPS fixes"));

	for (const DiveAndLocation &dl: fixes) {
		struct dive_site *ds = dl.d->dive_site;
		if (ds) {
			// Arbitrary choice: if we find multiple fixes for the same dive, we use the first one.
			if (std::find_if(siteLocations.begin(), siteLocations.end(),
					 [ds] (const SiteAndLocation &sl) { return sl.ds == ds; }) == siteLocations.end()) {
				siteLocations.push_back({ ds, dl.location });
			}
		} else {
			ds = divelog.sites.create(dl.name.toStdString());
			ds->location = dl.location;
			ds->add_dive(dl.d);
			dl.d->dive_site = nullptr; // This will be set on redo()
			sitesToAdd.emplace_back(ds);
		}
	}
}

bool ApplyGPSFixes::workToBeDone()
{
	return !sitesToAdd.empty() || !siteLocations.empty();
}

void ApplyGPSFixes::editDiveSites()
{
	for (SiteAndLocation &sl: siteLocations) {
		std::swap(sl.location, sl.ds->location);
		emit diveListNotifier.diveSiteChanged(sl.ds, LocationInformationModel::LOCATION); // Inform frontend of changed dive site.
	}
}

void ApplyGPSFixes::redo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
	editDiveSites();
}

void ApplyGPSFixes::undo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
	editDiveSites();
}

// AI-generated (Claude): bulk edit dive sites in a single undo step.
// On the first redo we snapshot the previous state of each site and overwrite
// per the user's edit spec. undo() and subsequent redo() simply swap each
// stored state with the live one, so we don't keep two copies.
EditDiveSitesBulk::EditDiveSitesBulk(const QVector<dive_site *> &sites, const BulkDiveSiteEdit &editIn)
	: edit(editIn)
{
	setText(Command::Base::tr("Bulk edit %n dive site(s)", "", sites.size()));
	savedState.reserve(sites.size());
	for (dive_site *ds: sites) {
		if (!ds)
			continue;
		savedState.push_back({ ds, ds->description, ds->notes, ds->taxonomy });
	}
}

bool EditDiveSitesBulk::workToBeDone()
{
	if (savedState.empty()) return false;
	if (edit.descriptionMode != BulkDiveSiteEdit::Mode::LeaveUnchanged) return true;
	if (edit.notesMode       != BulkDiveSiteEdit::Mode::LeaveUnchanged) return true;
	for (const auto &t: edit.taxonomy)
		if (t.second.mode != BulkDiveSiteEdit::Mode::LeaveUnchanged) return true;
	return false;
}

static std::string apply_text(BulkDiveSiteEdit::Mode mode, const std::string &cur, const QString &val)
{
	switch (mode) {
	case BulkDiveSiteEdit::Mode::LeaveUnchanged: return cur;
	case BulkDiveSiteEdit::Mode::Clear:          return {};
	case BulkDiveSiteEdit::Mode::Set:            return val.toStdString();
	case BulkDiveSiteEdit::Mode::Append: {
		std::string out = cur;
		if (!out.empty()) out += "\n";
		out += val.toStdString();
		return out;
	}}
	return cur;
}

void EditDiveSitesBulk::redo()
{
	for (auto &s: savedState) {
		if (!applied) {
			// Compute target values from the saved (= current) state.
			std::string newDesc  = apply_text(edit.descriptionMode, s.description, edit.descriptionValue);
			std::string newNotes = apply_text(edit.notesMode,       s.notes,       edit.notesValue);
			taxonomy_data newTax = s.taxonomy;
			for (const auto &[cat, t]: edit.taxonomy) {
				auto category = static_cast<enum taxonomy_category>(cat);
				if (t.mode == BulkDiveSiteEdit::Mode::Clear) {
					newTax.erase(std::remove_if(newTax.begin(), newTax.end(),
						[category](const taxonomy &x) { return x.category == category; }),
						newTax.end());
				} else if (t.mode == BulkDiveSiteEdit::Mode::Set) {
					taxonomy_set_category(newTax, category, t.value, GEOMANUAL);
				}
			}
			// Now savedState holds *new* values about to be swapped in;
			// after the swap, live site has new, savedState has old.
			s.description = std::move(newDesc);
			s.notes       = std::move(newNotes);
			s.taxonomy    = std::move(newTax);
		}
		std::swap(s.ds->description, s.description);
		std::swap(s.ds->notes,       s.notes);
		std::swap(s.ds->taxonomy,    s.taxonomy);
		emit diveListNotifier.diveSiteChanged(s.ds, LocationInformationModel::DESCRIPTION);
		emit diveListNotifier.diveSiteChanged(s.ds, LocationInformationModel::NOTES);
		emit diveListNotifier.diveSiteChanged(s.ds, LocationInformationModel::TAXONOMY);
	}
	applied = !applied;
}

void EditDiveSitesBulk::undo()
{
	// swap-based redo handles both directions.
	redo();
}

} // namespace Command
