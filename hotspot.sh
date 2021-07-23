#!/bin/bash

#######
#####  Matus' hotspot workaround for pinephone
####
#
# Fedora's NetworkManager assumes legacy iptables, not netfilter when ipv4 method is shared.
# Cannot fallback to legacy iptable since kernel module is missing for ip_tables, only netfilter is present.
# This script calls netfilter firewall properly.
#
# INSTALLATION:
# 1. is now obsolete.. continue with 2. ;)
# 2. Place this script in /etc/NetworkManager/dispatcher.d
# 3. Set file permission to root 600

HSUUID=$(grep uuid /etc/NetworkManager/system-connections/Hotspot.nmconnection | sed -e "s/^.*=//g")
if [[ "$CONNECTION_UUID" == "$HSUUID" ]]
then
    case "$2" in
        up)

        nft flush ruleset

        nft add table ip nat
        nft add table ip filter

        nft 'add chain ip filter INPUT { type filter hook input priority 0; }'
        nft 'add chain ip nat POSTROUTING { type nat hook postrouting priority 0; }'
        nft 'add chain ip filter FORWARD { type filter hook forward priority 0; }'

        nft insert rule ip nat POSTROUTING ip saddr 10.42.0.0/24 ip daddr != 10.42.0.0/24 counter masquerade  comment \"nm-shared-wlan0\"

        nft add chain ip filter nm-sh-in-wlan0
        nft add rule ip filter nm-sh-in-wlan0 tcp dport 67 counter accept
        nft add rule ip filter nm-sh-in-wlan0 udp dport 67 counter accept
        nft add rule ip filter nm-sh-in-wlan0 tcp dport 53 counter accept
        nft add rule ip filter nm-sh-in-wlan0 udp dport 53 counter accept

        nft add chain ip filter nm-sh-fw-wlan0
        nft add rule ip filter nm-sh-fw-wlan0 oifname "wlan0" ip daddr 10.42.0.0/24 ct state related,established  counter accept
        nft add rule ip filter nm-sh-fw-wlan0 iifname "wlan0" ip saddr 10.42.0.0/24 counter accept
        nft add rule ip filter nm-sh-fw-wlan0 iifname "wlan0" oifname "wlan0" counter accept
        nft add rule ip filter nm-sh-fw-wlan0 oifname "wlan0" counter reject
        nft add rule ip filter nm-sh-fw-wlan0 iifname "wlan0" counter reject

        nft insert rule ip filter INPUT iifname "wlan0" counter jump nm-sh-in-wlan0 comment \"nm-shared-wlan0\"
        nft insert rule ip filter FORWARD counter jump nm-sh-fw-wlan0 comment \"nm-shared-wlan0\"

        ;;

        down)
        
        nft flush ruleset

        ;;
    esac
fi

exit $?
