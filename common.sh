
taps() {
    cat <<EOF
USER	TAP	BRIDGE	  IP            NETMASK        VNC DISK1                                         DISK2
host    tap1    qemu-net  192.168.101.1 255.255.255.0  NA  NA                                            NA
one	tap11	qemu-net  set-inside-vm set-inside-vm  1   $HOME/vmdisks/20G-base-aoe-upgraded-one.img   NA
two	tap12	qemu-net  set-inside-vm set-inside-vm  2   $HOME/vmdisks/20G-base-aoe-upgraded-two.img   $HOME/vmdisks/10G-scratch.img
EOF
}

# look up methods for resource given the host/vm
tap() { taps | awk -vh="${1?}" '$1==h{print $2}'; }
tap_ip() {
    ip=`taps | awk -vh="${1?}" '$1==h{print $4}'`
    if test x"$ip" = x"set-inside-vm"; then
	ip=
    fi
    echo "$ip"
}
tap_mask() { taps | awk -vh="${1?}" '$1==h{print $5}'; }
vnc_screen() { taps | awk -vh="${1?}" '$1==h{print $6}'; }
disk_image() { taps | awk -vh="${1?}" '$1==h{print $7}'; }
disk2_image() { taps | awk -vh="${1?}" '$1==h{print $8}'; }
bridge() { taps | awk -vh="${1?}" '$1==h{print $3}'; }

# returns "yes" if host is known
valid() { taps | awk -vh="${1?}" '$1==h{print "yes"}'; }

assertions() {
    if test x`valid "${1?}"` != x"yes"; then
	echo invalid host specified 1>&2
	exit 1
    fi
}

ensure_bridge() {
    br="${1?}"
    if ! sudo brctl showstp "$br" > /dev/null 2>&1; then
	echo sudo brctl addbr "$br"
	echo sudo ifconfig "$br" up
    fi
}

ensure_tap() {
    tap=`tap "${1?}"`
    br=`bridge "${1?}"`
    echo sudo tunctl -t "$tap" -u `whoami`
    echo sudo ifconfig "$tap" `tap_ip "${1?}"` up
    echo sudo brctl addif "$br" "$tap"
}

ensure_host() {
    tap=`tap host`
    br=`bridge host`
    ensure_bridge "$br"
    ensure_tap host
    echo sudo ifconfig "$tap" `tap_ip host` netmask `tap_mask host` up
}
