# Scripts for Qemu Use

Currently commands are printed to stdout rather than being
run directly.

## Preparation

Use the qemu-img tool to create disk images like the examples
named in common.sh's embedded database.

## Usage

    ./launch one   # (copy and paste commands at will)
    ./launch two

## Host tap1

Right now I can ping the host platform's tap1 address from
inside the VMs, but the host cannot ping the VMs.  The VM
can see the host's wlan0 IP sending ARP requests, though.

The VMs can communicate between themselves without trouble.

