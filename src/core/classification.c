/**
 * @file
 * @brief Classify platform evidence without inventing physical media guarantees.
 */
#include "disksan/device.h"

/** @brief Derive role, transport, protocol, and heuristic media; preserve uncertainty through warnings. */
int ds_classify(ds_device *d, const ds_facts *f)
{
    d->role = d->transport = d->protocol = d->media = "unknown";
    d->media_confidence = "unknown";
    d->classification_source = "platform_metadata";
    bool conflict = (f->nvme && (f->ata || f->virtio)) ||
                    (f->virtio && (f->usb || f->ata));
    if (f->snapshot) d->role = "snapshot";
    else if (f->volume) d->role = "volume";
    else if (f->partition) d->role = "partition";
    else if (f->composite) d->role = "composite";
    else if (f->virtual_device || f->virtio) d->role = "virtual";
    else if (f->nvme && !f->usb && !conflict) d->role = "namespace";
    else if (f->disk) d->role = "disk";

    if (conflict) return ds_warn(d, "classification_conflict");
    if (f->usb) {
        d->transport = "usb";
        /* USB SCSI access does not establish the media's native protocol. */
        if (f->ata) d->protocol = "ata";
        if (f->nvme) d->protocol = "nvme";
        if (ds_warn(d, "usb_bridge_identity_and_passthrough_unverified") < 0) return -1;
    } else if (f->virtio) {
        d->transport = "virtio";
    } else if (f->nvme) {
        d->transport = d->protocol = "nvme";
    } else if (f->ata) {
        d->transport = d->protocol = "ata";
    } else if (f->scsi) {
        d->transport = d->protocol = "scsi";
    } else if (f->mmc) {
        d->transport = "mmc";
    }

    if (f->virtual_device || f->virtio || f->composite || f->volume || f->snapshot) {
        return ds_warn(d, "physical_media_not_identified");
    }
    if (f->partition) return ds_warn(d, "partition_not_whole_disk");
    if (f->optical) return ds_warn(d, "optical_media_not_classified");
    if (!f->usb && f->mmc) {
        d->media = "flash";
        d->media_confidence = "heuristic";
    } else if (!f->usb && d->rotational.known && (f->nvme || f->ata || f->scsi || f->media_reported)) {
        if (f->nvme && d->rotational.value != 0U) return ds_warn(d, "classification_conflict");
        d->media = d->rotational.value == 0U ? "ssd" : "hdd";
        d->media_confidence = "heuristic";
    }
    if (d->media_confidence[0] == 'h') return ds_warn(d, "media_is_heuristic");
    return ds_warn(d, "physical_media_not_identified");
}
