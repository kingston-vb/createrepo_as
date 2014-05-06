createrepo_as
=============

createrepo_as is a tool that allows us to create AppStream metadata from a
directory of packages.
It is typically used when generating distribution metadata, usually at the same
time as modifyrepo or createrepo.

What this tool does:

 * Searches a directory of packages and reads just the RPM header of each.
 * If a package contains an interesting file, just the relevant files are
   decompressed from the package archive.
 * A set of plugins are run on the extracted files, and if these match certain
   criteria `CraApplication` objects are created.
 * Any screenshots referenced are downloaded to a local cache.
   This is optional and can be disabled with `--nonet`.
 * When all the packages are processed, some of the `CraApplication` objects are
   merged into single applications. This is how fonts are collected.
 * The `CraApplication` objects are serialized to XML and written to a
   compressed archive.
 * Any application icons or screenshots referenced are written to a .tar archive.

Getting Started
-----------

To run createrepo_as you either need to install the package containing the
binary and data files, or you can build a local copy. To do the latter just do:

    dnf install automake autoconf libtool rpm-devel \
                gtk3-devel sqlite-devel libsoup-devel
    ./autogen.sh
    make

To actually run the extractor you can do:

    ./createrepo_as --verbose   \
                    --max-threads=8 \
                    --log-dir=/tmp/logs \
                    --packages-dir=/mnt/archive/Megarpms/21/Packages \
                    --temp-dir=/mnt/ssd/AppStream/tmp \
                    --output-dir=./repodata \
                    --screenshot-url=http://megarpms.org/screenshots/ \
                    --basename="megarpms-21"

This will output a lot of progress text. Now, go and make a cup of tea and wait
patiently if you have a lot of packages to process. After this is complete
you should finally see:

    Writing ./repodata/megarpms-21.xml.gz
    Writing ./repodata/megarpms-21-icons.tar
    Done!

You now have two choices what to do with these files. You can either upload
them with the rest of the metadata you ship (e.g. in the same directory as
`repomd.xml` and `primary.sqlite.bz2`) which will work with Fedora 21.

For Fedora 20, you have to actually install these files, so you can do something
like this in the megarpms-release.spec file:

    Source1:   http://www.megarpms.org/temp/megarpms-20.xml.gz
    Source2:   http://www.megarpms.org/temp/megarpms-20-icons.tar.gz

    mkdir -p %{buildroot}%{_datadir}/app-info/xmls
    cp %{SOURCE1} %{buildroot}%{_datadir}/app-info/xmls
    mkdir -p %{buildroot}%{_datadir}/app-info/icons/megarpms-20
    tar xvzf %{SOURCE2}
    cd -

This ensures that gnome-software can access both data files when starting up.

What is an application
-----------

Applications are defined in the context of AppStream as such:

 * Installs a desktop file and would be visible in a desktop
 * Has an metadata extractor (e.g. src/plugins/cra-plugin-gstreamer.c) and
   includes an AppData file

Guidelines for applications
-----------

These guidelines explain how we filter applications from a package set.

First, some key words:
 * **SHOULD**: The application should do this if possible
 * **MUST**: The application or addon must do this to be included
 * **CANNOT**: the application or addon must not do this

The current rules of inclusion are thus:

 * Icons **MUST** be installed in `/usr/share/pixmaps/*`, `/usr/share/icons/*`,
   `/usr/share/icons/hicolor/*/apps/*`, or `/usr/share/${app_name}/icons/*`
 * Desktop files **MUST** be installed in `/usr/share/applications/`
   or `/usr/share/applications/kde4/`
 * Desktop files **MUST** have `Name`, `Comment` and `Icon` entries
 * Valid applications with `NoDisplay=true` **MUST** have an AppData file.
 * Applications with `Categories=Settings` or `Categories=DesktopSettings`
   **MUST** have an AppData file.
 * Applications **MUST** have had an upstream release in the last 5 years or
   have an AppData file.
 * Application icon **MUST** be available in 48x48 or larger
 * Codecs **MUST** have an AppData file
 * Input methods **MUST** have an AppData file
 * If included, AppData files **MUST** be valid XML
 * AppData files **MUST** be installed into `/usr/share/appdata`
 * Application icons **CANNOT** use XPM or ICO format
 * Applications **CANNOT** use obsolete toolkits such as GTK+-1.2 or QT3
 * Applications that ship a desktop file **SHOULD** include an AppData file.
 * Screenshots **SHOULD** be in 16:9 aspect ratio
 * Application icons **SHOULD** have an alpha channel
 * Applications **SHOULD** ship a 64x64 PNG format icon or SVG
 * AppData files **SHOULD** include translations
 * Desktop files **SHOULD** include translations

License
----

GPLv2+
