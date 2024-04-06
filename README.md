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
Usage: ./wgm [iface|peer] [options]

Commands:
  iface - Manage WireGuard interfaces
  peer  - Manage WireGuard peers
```

# iface subcommands
```txt
$ ./wgm iface
Usage: ./wgm iface [add|del|show|update|list] [OPTIONS]

Commands:
  add    - Add a new WireGuard interface
  del    - Delete an existing WireGuard interface
  show   - Show information about a WireGuard interface
  update - Update an existing WireGuard interface
  list   - List all WireGuard interfaces (no options required)

Options:
  -d, --dev <name>          Interface name
  -l, --listen-port <port>  Listen port
  -k, --private-key <key>   Private key
  -a, --address <addr>      Interface address
  -m, --mtu <size>          MTU size
  -i, --allowed-ips <ips>   Allowed IPs
  -h, --help                Show this help message
  -f, --force               Force operation

```

# peer subcommands
```txt
$ ./wgm peer
Usage: wgm peer [add|del|show|update|list] [OPTIONS]

Commands:
  add    - Add a new peer to a WireGuard interface
  del    - Delete an existing peer from a WireGuard interface
  show   - Show information about a peer in a WireGuard interface
  update - Update an existing peer in a WireGuard interface
  list   - List all peers in a WireGuard interface

Options:
  -d, --dev         Interface name
  -p, --public-key  Public key of the peer
  -e, --endpoint    Endpoint of the peer
  -b, --bind-ip     Bind IP of the peer
  -a, --allowed-ips Allowed IPs of the peer
  -f, --force       Force the operation
  -h, --help        Show this help message

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

### 2. Update the MTU size of an interface
```txt
./wgm iface update \
    --dev wgm0 \
    --mtu 1500;
```

### 3. Show information about an interface
```txt
./wgm iface show --dev wgm0;
```

### 4. List all interfaces
```txt
./wgm iface list;
```

### 5. Delete an interface
```txt
./wgm iface del --dev wgm0;
```
