// SPDX-License-Identifier: GPL-2.0-only

#include <wgm/server.hpp>
#include <wgm/ctx.hpp>
#include <cstdio>
#include <cstdlib>

namespace wgm {

using json = nlohmann::json;

server::server(const json &j, ctx *ctx):
	ctx_(ctx)
{
	IPRelay_		= j["IPRelay"];
	LocationRelay_		= j["LocationRelay"];
	Location_		= j["Location"];
	Country_		= j["Country"];
	City_			= j["City"];
	LocalIP_		= j["LocalIP"];
	Socks5Port_		= j["Socks5Port"];
	WireguardPort_		= j["WireguardPort"];
	WireguardSubnet_	= j["WireguardSubnet"];
	MTU_			= j["MTU"];
	PrivateKey_		= j["PrivateKey"];
	PublicKey_		= j["PublicKey"];
	PresharedKey_		= j["PresharedKey"];
}

server::~server(void) = default;

void server::add_client(const client &c)
{
	clients_.emplace(c.WireguardID(), c);
}

std::string server::gen_wg_config(std::string ipt_path, std::string ip2_path,
				  std::string true_path)
{
	std::string def_gateway;
	std::string rt_table;

	std::string ret = "";
	uint64_t mss_lb = MTU_ - 39;
	uint64_t mss_ub = 65535;
	uint64_t target_mss = MTU_ - 40;
	size_t i = 0;
	uint32_t mark = 1;
	
	try {
		json j = ctx_->get_wg_conn_by_local_interface_ip(LocalIP_);
		char *endptr;
		uint32_t tmp;

		rt_table = j["local_interface_route_table_name"];
		def_gateway = j["local_interface_ip_veth"];

		tmp = strtoul(j["local_interface_route_table_id"].get<std::string>().c_str(), &endptr, 10);
		if (*endptr != '\0')
			throw std::runtime_error("Failed to convert local_interface_route_table_id to integer.");

		mark = tmp + 22222;
	} catch (const std::exception &e) {
		throw std::runtime_error("Failed to get Wireguard connection file: " + std::string(e.what()));
	}


	ret += "[Interface]\n";
	ret += "# PublicKey = " + PublicKey_ + "\n";
	ret += "MTU = " + std::to_string(MTU_) + "\n";
	ret += "PrivateKey = " + PrivateKey_ + "\n";
	ret += "Table = off\n";
	ret += "Address = " + WireguardSubnet_ + "\n";
	ret += "ListenPort = " + std::to_string(WireguardPort_) + "\n";
	ret += "\n";

	ret += "# Clear all previous rules if exist.\n";
	for (i = 0; i < 3; i++) {
		ret += "PostUp   = (" + ipt_path + " -t filter -D FORWARD     -j wgm_mssc_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
		ret += "PostUp   = (" + ipt_path + " -t nat    -D POSTROUTING -j wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
		ret += "PostUp   = (" + ipt_path + " -t mangle -D PREROUTING  -j wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
		// ret += "PostUp   = (" + ip2_path + " rule del fwmark " + std::to_string(mark) + " table " + std::to_string(mark) + " || " + true_path + ") >> /dev/null 2>&1;\n";
		ret += "PostUp   = (" + ip2_path + " rule del fwmark " + std::to_string(mark) + " table " + rt_table + " || " + true_path + ") >> /dev/null 2>&1;\n";
	}
	ret += "\n";
	ret += "PostUp   = (" + ipt_path + " -t filter -F wgm_mssc_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostUp   = (" + ipt_path + " -t nat    -F wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostUp   = (" + ipt_path + " -t mangle -F wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "\n";
	ret += "PostUp   = (" + ipt_path + " -t filter -X wgm_mssc_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostUp   = (" + ipt_path + " -t nat    -X wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostUp   = (" + ipt_path + " -t mangle -X wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";

	ret += "\n";
	ret += "### Start iproute2 rules. ###\n";
	ret += "PostUp   = " + ip2_path + " rule add fwmark " + std::to_string(mark) + " table " + rt_table + ";\n";
	// ret += "PostUp   = " + ip2_path + " rule add fwmark " + std::to_string(mark) + " table " + std::to_string(mark) + ";\n";
	// ret += "PostUp   = " + ip2_path + " route replace default via " + def_gateway + " table " + std::to_string(mark) + ";\n";
	ret += "### End iproute2 rules. ###\n\n";

	ret += "### Start iptables rules. ###\n";
	ret += "PostUp   = " + ipt_path + " -t filter -N wgm_mssc_" + Location_ + ";\n";
	ret += "PostUp   = " + ipt_path + " -t nat    -N wgm_" + Location_ + ";\n";
	ret += "PostUp   = " + ipt_path + " -t mangle -N wgm_" + Location_ + ";\n";
	ret += "\n";
	ret += "PostUp   = " + ipt_path + " -t filter -A wgm_mssc_" + Location_ + " ! -s " + WireguardSubnet_ + " ! -d " + WireguardSubnet_ + " -j RETURN;\n";
	ret += "PostUp   = " + ipt_path + " -t filter -A wgm_mssc_" + Location_ + " -p tcp --tcp-flags SYN,RST SYN -m tcpmss --mss " + std::to_string(mss_lb) + ":" + std::to_string(mss_ub) + " -j TCPMSS --set-mss " + std::to_string(target_mss) + ";\n";
	ret += "PostUp   = " + ipt_path + " -t filter -A wgm_mssc_" + Location_ + " -j ACCEPT;\n";
	ret += "\n";
	ret += "PostUp   = " + ipt_path + " -t nat    -A wgm_" + Location_ + " -s " + WireguardSubnet_ + " ! -d " + WireguardSubnet_ + " -j SNAT --to-source " + LocalIP_ + ";\n";
	ret += "PostUp   = " + ipt_path + " -t nat    -A wgm_" + Location_ + " -j RETURN;\n";
	ret += "\n";
	ret += "PostUp   = " + ipt_path + " -t mangle -A wgm_" + Location_ + " -s " + WireguardSubnet_ + " ! -d " + WireguardSubnet_ + " -j MARK --set-mark " + std::to_string(mark) + ";\n";
	ret += "PostUp   = " + ipt_path + " -t mangle -A wgm_" + Location_ + " -j RETURN;\n";
	ret += "\n";
	ret += "PostUp   = " + ipt_path + " -t filter -I FORWARD     -j wgm_mssc_" + Location_ + ";\n";
	ret += "PostUp   = " + ipt_path + " -t nat    -I POSTROUTING -j wgm_" + Location_ + ";\n";
	ret += "PostUp   = " + ipt_path + " -t mangle -I PREROUTING  -j wgm_" + Location_ + ";\n";
	ret += "### End iptables rules. ###\n";
	ret += "\n";
	ret += "# Clear all rules.\n";
	ret += "PostDown = (" + ipt_path + " -t filter -D FORWARD     -j wgm_mssc_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t nat    -D POSTROUTING -j wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t mangle -D PREROUTING  -j wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t filter -F wgm_mssc_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t nat    -F wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t mangle -F wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t filter -X wgm_mssc_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t nat    -X wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ipt_path + " -t mangle -X wgm_" + Location_ + " || " + true_path + ") >> /dev/null 2>&1;\n";
	// ret += "PostDown = (" + ip2_path + " rule del fwmark " + std::to_string(mark) + " table " + std::to_string(mark) + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "PostDown = (" + ip2_path + " rule del fwmark " + std::to_string(mark) + " table " + rt_table + " || " + true_path + ") >> /dev/null 2>&1;\n";
	ret += "\n\n";
	for (const auto &i : clients_) {
		ret += "# " + i.first + "\n";
		ret += "[Peer]\n";
		ret += "PresharedKey = " + PresharedKey_ + "\n";
		ret += "PublicKey = " + i.second.PublicKey() + "\n";
		ret += "AllowedIPs = " + i.second.LocalIP() + "/32\n";
		ret += "\n";
	}

	return ret;
}

} /* namespace wgm */
