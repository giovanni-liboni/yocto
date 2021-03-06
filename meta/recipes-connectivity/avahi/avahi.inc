SUMMARY = "Avahi IPv4LL network address configuration daemon"
DESCRIPTION = 'Avahi is a fully LGPL framework for Multicast DNS Service Discovery. It \
allows programs to publish and discover services and hosts running on a local network \
with no specific configuration. This tool implements IPv4LL, "Dynamic Configuration of \
IPv4 Link-Local Addresses" (IETF RFC3927), a protocol for automatic IP address \
configuration from the link-local 169.254.0.0/16 range without the need for a central \
server.'
AUTHOR = "Lennart Poettering <lennart@poettering.net>"
HOMEPAGE = "http://avahi.org"
BUGTRACKER = "http://avahi.org/report"
SECTION = "network"

# major part is under LGPLv2.1+, but several .dtd, .xsl, initscripts and
# python scripts are under GPLv2+
LICENSE = "GPLv2+ & LGPLv2.1+"

INC_PR = "r11"

DEPENDS = "expat libcap libdaemon dbus glib-2.0"

SRC_URI = "http://avahi.org/download/avahi-${PV}.tar.gz \
          file://00avahi-autoipd \
          file://99avahi-autoipd \
          file://initscript.patch \
          file://avahi_fix_install_issue.patch \
          file://fix_for_automake_1.12.x.patch \
          file://out-of-tree.patch \
          file://0001-avahi-fix-avahi-status-command-error-prompt.patch \
          file://reuseport-check.patch \
          "

USERADD_PACKAGES = "avahi-daemon avahi-autoipd"
USERADD_PARAM_avahi-daemon = "--system --home /var/run/avahi-daemon \
                              --no-create-home --shell /bin/false \
                              --user-group avahi"

USERADD_PARAM_avahi-autoipd = "--system --home /var/run/avahi-autoipd \
                              --no-create-home --shell /bin/false \
                              --user-group \
                              -c \"Avahi autoip daemon\" \
                              avahi-autoipd"

inherit autotools pkgconfig update-rc.d gettext useradd

EXTRA_OECONF = "--disable-introspection \
             --with-avahi-priv-access-group=adm \
             --disable-stack-protector \
             --disable-gdbm \
             --disable-mono \
             --disable-monodoc \
             --disable-qt3 \
             --disable-qt4 \
             --disable-python \
             --disable-doxygen-doc \
             ${EXTRA_OECONF_SYSVINIT} \
             ${EXTRA_OECONF_SYSTEMD} \
             ${AVAHI_GTK} \
           "

# The distro choice determines what init scripts are installed
EXTRA_OECONF_SYSVINIT = "${@bb.utils.contains('DISTRO_FEATURES','sysvinit','--with-distro=debian','--with-distro=none',d)}"
EXTRA_OECONF_SYSTEMD = "${@bb.utils.contains('DISTRO_FEATURES','systemd','--with-systemdsystemunitdir=${systemd_unitdir}/system/','--without-systemdsystemunitdir',d)}"

AVAHI_GTK ?= "--disable-gtk --disable-gtk3"

LDFLAGS_append_libc-uclibc = " -lintl"
LDFLAGS_append_uclinux-uclibc = " -lintl"

do_configure_prepend() {
    sed 's:AM_CHECK_PYMOD:echo "no pymod" #AM_CHECK_PYMOD:g' -i ${S}/configure.ac
}


PACKAGES =+ "avahi-daemon libavahi-common libavahi-core libavahi-client avahi-dnsconfd libavahi-glib libavahi-ui avahi-autoipd avahi-utils"

FILES_avahi-autoipd = "${sbindir}/avahi-autoipd \
                       ${sysconfdir}/avahi/avahi-autoipd.action \
                       ${sysconfdir}/dhcp/*/avahi-autoipd \
                       ${sysconfdir}/udhcpc.d/00avahi-autoipd \
                       ${sysconfdir}/udhcpc.d/99avahi-autoipd"
FILES_libavahi-common = "${libdir}/libavahi-common.so.*"
FILES_libavahi-core = "${libdir}/libavahi-core.so.*"
FILES_avahi-daemon = "${sbindir}/avahi-daemon \
                      ${sysconfdir}/avahi/avahi-daemon.conf \
                      ${sysconfdir}/avahi/hosts \
                      ${sysconfdir}/avahi/services \
                      ${sysconfdir}/dbus-1 \
                      ${sysconfdir}/init.d/avahi-daemon \
                      ${datadir}/avahi/introspection/*.introspect \
                      ${datadir}/avahi/avahi-service.dtd \
                      ${datadir}/avahi/service-types \
                      ${datadir}/dbus-1/system-services"
FILES_libavahi-client = "${libdir}/libavahi-client.so.*"
FILES_libavahi-ui = "${libdir}/libavahi-ui.so.*"
FILES_avahi-dnsconfd = "${sbindir}/avahi-dnsconfd \
                        ${sysconfdir}/avahi/avahi-dnsconfd.action \
                        ${sysconfdir}/init.d/avahi-dnsconfd"
FILES_libavahi-glib = "${libdir}/libavahi-glib.so.*"
FILES_libavahi-gobject = "${libdir}/libavahi-gobject.so.*"
FILES_avahi-utils = "${bindir}/avahi-*"

# uclibc has no nss
RRECOMMENDS_avahi-daemon_append_libc-glibc = " libnss-mdns"
RRECOMMENDS_${PN}_append_libc-glibc = " libnss-mdns"

RRECOMMENDS_avahi-dev = "expat-dev libcap-dev libdaemon-dev dbus-dev glib-2.0-dev update-rc.d-dev"
RRECOMMENDS_avahi-dev_append_libc-glibc = " gettext-dev"

RRECOMMENDS_avahi-dev[nodeprrecs] = "1"

CONFFILES_avahi-daemon = "${sysconfdir}/avahi/avahi-daemon.conf"

INITSCRIPT_PACKAGES = "avahi-daemon avahi-dnsconfd"
INITSCRIPT_NAME_avahi-daemon = "avahi-daemon"
INITSCRIPT_PARAMS_avahi-daemon = "defaults 21 19"
INITSCRIPT_NAME_avahi-dnsconfd = "avahi-dnsconfd"
INITSCRIPT_PARAMS_avahi-dnsconfd = "defaults 22 19"

do_install() {
	autotools_do_install

	# don't install /var/run when populating rootfs. Do it through volatile
	# /var/run of current version is empty, so just remove it.
	# if /var/run become non-empty in the future, need to install it via volatile
	rm -rf ${D}/var/run
	rm -rf ${D}${datadir}/dbus-1/interfaces
	rmdir --ignore-fail-on-non-empty ${D}${datadir}/dbus-1
	rm -rf ${D}${libdir}/avahi

	install -d ${D}${sysconfdir}/udhcpc.d
	install ${WORKDIR}/00avahi-autoipd ${D}${sysconfdir}/udhcpc.d
	install ${WORKDIR}/99avahi-autoipd ${D}${sysconfdir}/udhcpc.d
}

# At the time the postinst runs, dbus might not be setup so only restart if running 
# Don't exit early, because update-rc.d needs to run subsequently.

pkg_postinst_avahi-daemon () {
if [ -z "$D" ]; then
	killall -q -HUP dbus-daemon || true
fi
}

pkg_postrm_avahi-daemon () {
	deluser avahi || true
	delgroup avahi || true
}

pkg_postrm_avahi-autoipd () {
	deluser avahi-autoipd || true
	delgroup avahi-autoipd || true
}
