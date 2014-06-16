#sudo mount -t tmpfs -o size=1G,nr_inodes=5k tmpfs /mnt/
time src/createrepo_as \
	--api-version=0.7 \
	--use-package-cache \
	--log-dir=../createrepo_as_logs \
	--temp-dir=./contrib/tmp \
	--cache-dir=./contrib/cache \
	--packages-dir=../fedora-appstream/fedora-rawhide/packages/ \
	--extra-appstream-dir=../fedora-appstream/appstream-extra \
	--extra-appdata-dir=../fedora-appstream/appdata-extra \
	--extra-screenshots-dir=../fedora-appstream/screenshots-extra \
	--output-dir=./contrib \
	--basename=fedora-21 \
	--screenshot-uri=http://alt.fedoraproject.org/pub/alt/screenshots/f21/
