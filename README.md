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

Option `--dev` is required to identify the interface to be updated.
```txt
./wgm iface update --dev wgm0 --mtu 1500;
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

Option `--dev` is required to identify the interface to be deleted.
```txt
./wgm iface del --dev wgm0;
```

### 6. Update the private key of an interface

Option `--dev` is required to identify the interface to be updated.
```txt
./wgm iface update \
    --dev wgm0 \
    --private-key "OB5yPRVxfOkp0YZL9FPy4HzFIEZpT/WblEc2eistaVA=";
```

### 7. Update many options of an interface at once.

Option `--dev` is required to identify the interface to be updated.
```txt
./wgm iface update \
    --dev wgm0 \
    --mtu 1420 \
    --listen-port 443 \
    --private-key "EDVfpFI5OcH2Jd0VtK9zlXPqhZaQ77NwnC4eHKHRaU8=";
```

# peer command examples

### 1. Add a new peer to an interface

Option `--dev` is required to select the interface where the peer will be added.
```txt
./wgm peer add \
    --dev wgm0 \
    --public-key "O3mF2EK82IpXaxyaDY50Jkuoes/IzNc42tD8ffYlyBo=" \
    --allowed-ips "10.45.0.2/32" \
```

### 2. Update the allowed IPs of a peer

Option `--dev` and `--public-key` are required to identify the peer.
```txt
./wgm peer update \
    --dev wgm0 \
    --public-key "O3mF2EK82IpXaxyaDY50Jkuoes/IzNc42tD8ffYlyBo=" \
    --allowed-ips "10.45.0.3/32";
```

### 3. Show information about a peer

Option `--dev` and `--public-key` are required to identify the peer.
```txt
./wgm peer show \
    --dev wgm0 \
    --public-key "O3mF2EK82IpXaxyaDY50Jkuoes/IzNc42tD8ffYlyBo=";
```

### 4. List all peers in an interface

Option `--dev` is required to identify the interface.
```txt
./wgm peer list --dev wgm0;
```

### 5. Delete a peer from an interface

Option `--dev` and `--public-key` are required to identify the peer.
```txt
./wgm peer del \
    --dev wgm0 \
    --public-key "O3mF2EK82IpXaxyaDY50Jkuoes/IzNc42tD8ffYlyBo=";
```
