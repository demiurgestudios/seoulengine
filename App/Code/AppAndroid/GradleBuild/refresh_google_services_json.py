"""
Refreshes resource values from google-services.json

Copyright (c) Demiurge Studios, Inc.

This source code is licensed under the MIT license.
Full license details can be found in the LICENSE file
in the root directory of this source tree.

Based on instructions for generating resource files without using the Google
Services gradle plugin:
https://developers.google.com/android/guides/google-services-plugin#processing_the_json_file
"""
import errno
import json
import os
from os import path


CLIENT_ID = "com.demiurgestudios.setestapp"
BUILD_TYPE = "debug"


def load_services_json(wd, client_id):
    """
    Load {wd}/google-services.json
    """
    with open(path.join(wd, "google-services.json")) as f:
        data = json.load(f)

    for client in data["client"]:
        package_name = client["client_info"]["android_client_info"]["package_name"]
        if package_name == client_id:
            return client, data["project_info"]

    raise Exception("Client \"%s\" not found" % client_id)


def get_matching(objects, match):
    """
    Returns the first object in objects with all key/value pairs from match
    """
    for o in objects:
        is_match = True
        for k, v in match.iteritems():
            if o.get(k) != v:
                is_match = False
                break
        if is_match:
            return o

    raise Exception("Could not find object: %s", match)


def get_current_api_key(client):
    """
    Returns the first api_key element with a current_key
    """
    for key in client["api_key"]:
        k = key.get("current_key")
        if k:
            return k

    raise Exception("No current api key")


def build_values_xml(wd, client, project):
    """
    Build values.xml
    """
    oauth_client = get_matching(client["oauth_client"], {"client_type": 3})
    api_key = get_current_api_key(client)

    xml = VALUES_XML_TEMPLATE.format(
        google_app_id=client["client_info"]["mobilesdk_app_id"],
        gcm_default_sender_id=project["project_number"],
        default_web_client_id=oauth_client["client_id"],
        firebase_db_url=project["firebase_url"],
        google_api_key=api_key,
        google_crash_reporting_api_key=api_key,
        project_id=project["project_id"],
    )

    xml_path = path.join(wd, "res", "values", "google_services.xml")
    with open(xml_path, "w") as f:
        f.write(xml)


def main():
    """
    Read google-services.json, and write values.xml and global_tracker.xml resources
    """
    wd = path.dirname(__file__)
    client, project = load_services_json(wd, CLIENT_ID)

    build_values_xml(wd, client, project)


VALUES_XML_TEMPLATE = """
<?xml version="1.0" encoding="utf-8"?>
<resources>

    <!-- Present in all applications -->
    <string name="google_app_id" translatable="false">{google_app_id}</string>

    <!-- Present in applications with the appropriate services configured -->
    <string name="gcm_defaultSenderId" translatable="false">{gcm_default_sender_id}</string>
    <string name="default_web_client_id" translatable="false">{default_web_client_id}</string>
    <string name="firebase_database_url" translatable="false">{firebase_db_url}</string>
    <string name="google_api_key" translatable="false">{google_api_key}</string>
    <string name="google_crash_reporting_api_key" translatable="false">{google_crash_reporting_api_key}</string>
    <string name="project_id" translatable="false">{project_id}</string>

</resources>
""".strip()


if __name__ == "__main__":
    main()
