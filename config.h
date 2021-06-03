#include "logos/arch_big.h"
#define COLOR "\e[1;36m"

#define TEMP_CACHE_FILE 1

#define CONFIG \
{ \
	/* name	          function                 cached */\
	{ "",             get_title,               false , 0}, \
	{ "",             get_bar,                 false , 0}, \
	{ "OS: ",         get_os,                  true  , 0}, \
/*	{ "Host: ",       get_host,                true  , 0}, */\
	{ "Kernel: ",     get_kernel,              true  , 0}, \
	{ "Uptime: ",     get_uptime,              false , 0}, \
/*	{ "Battery: ",    get_battery_percentage,  false , 0}, */\
	{ "",             get_bar,                 false , 23}, \
	{ "Packages: ",   get_packages_pacman,     false , 0}, \
	{ "Shell: ",      get_shell,               false , 0}, \
	{ "Resolution: ", get_resolution,          false , 0}, \
	{ "Terminal: ",   get_terminal,            false , 0}, \
	{ "",             get_bar,                 false , 21}, \
	{ "CPU: ",        get_cpu,                 true  , 0}, \
	{ "GPU: ",        get_gpu1,                true  , 0}, \
	{ "Memory: ",     get_memory,              false , 0}, \
	{ "",             get_bar,                 false , 26}, \
	{ "",             get_colors1,             false , 0}, \
	{ "",             get_colors2,             false , 0}, \
}

#define CPU_CONFIG \
{ \
	REMOVE("(R)"), \
	REMOVE("(TM)"), \
	REMOVE("Dual-Core"), \
	REMOVE("Quad-Core"), \
	REMOVE("Six-Core"), \
	REMOVE("Eight-Core"), \
	REMOVE("Core"), \
	REMOVE("CPU"), \
}

#define GPU_CONFIG \
{ \
	REMOVE("Corporation"), \
}

/* GOAL:
print_info() {
	info title
	info underline
	info cols

	info "OS" distro
	info "Kernel" kernel
	info "WM" wm
	info "Terminal" term
	info "Shell" shell
	info underline

	info "Users" users
	info "Local IP" local_ip
	info "Uptime" uptime
	prin "Date" "$(date)"
	info "Packages" packages
	prin "Explicit" "$(pacman -Qe | wc -l)"
	info underline

	info "CPU" cpu
	info "CPU Usage" cpu_usage
	info "GPU" gpu
	info "Monitors" resolution
	info "Memory" memory
	info "Disk" disk
	info underline

	info "Theme" theme
	info "Icons" icons
#	info "Terminal Font" term_font
	info "Font" font
	prin "Keys" "$(setxkbmap -query | grep layout | cut -b 13- | sed 's/,/ /g')"
	info underline
}
*/
