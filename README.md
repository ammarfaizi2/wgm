# Wireguard Manager

A simple program to manage multiple wireguard interfaces and peers.

# Build

```txt
sudo apt install -y gcc make libjson-c-dev git;
git clone https://github.com/ammarfaizi2/wg-mgr.git;
cd wg-mgr;
make -j4;
```

# Commands
```txt
$ ./wgm
Usage: ./wgm [command] [options]

Commands:
  iface - Manage WireGuard interfaces
  peer  - Manage WireGuard peers
```

# iface subcommands
```txt
$ ./wgm iface
Usage: ./wgm iface [command] [options]

Commands:
  add    - Add a new WireGuard interface
  del    - Delete an existing WireGuard interface
  show   - Show information about a WireGuard interface
  update - Update an existing WireGuard interface
  list   - List all WireGuard interfaces

```

# peer subcommands
```txt
$ ./wgm peer
Usage: ./wgm peer [command] [options]

Commands:
  add    - Add a new peer to a WireGuard interface
  del    - Delete an existing peer from a WireGuard interface
  show   - Show information about a peer in a WireGuard interface
  update - Update an existing peer in a WireGuard interface
  list   - List all peers in a WireGuard interface

```

# iface command examples
### 1. Add a new interface
```txt
./wgm iface add \
    --force \
    --dev wgm0 \
    --mtu 1420 \
    --listen-port 443 \
    --private-key "EDVfpFI5OcH2Jd0VtK9zlXPqhZaQ77NwnC4eHKHRaU8=" \
    --address "10.45.0.1/24" \
    --allowed-ips "0.0.0.0/0,::/0";
```
