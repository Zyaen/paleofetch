#include "logos/arch_big.h"
#define COLOR "\e[1;36m"

#define TEMP_CACHE_FILE 1

#define CONFIG \
{ \
	/* name	          function                 cached */\
	{ "",             get_title,               false , 0}, \
	{ "",             get_bar,                 false , 0}, \
	SPACER \
	{ "",             get_colors1,             false , 0}, \
	{ "",             get_colors2,             false , 0}, \
	SPACER \
	{ "OS: ",         get_os,                  true  , 0}, \
/*	{ "Host: ",       get_host,                true  , 0}, */\
	{ "Kernel: ",     get_kernel,              true  , 0}, \
	{ "WM: ",         run_shell_cmd,           true  , "xprop -id `xprop -root -notype _NET_SUPPORTING_WM_CHECK | awk '{print $5}'` -notype -len 100 -f _NET_WM_NAME 8t | head -n 1 | awk '{print $3}' | tr -d '\"'"}, \
	{ "Shell: ",      get_shell,               false , 0}, \
	{ "Terminal: ",   get_terminal,            false , 0}, \
/*	{ "Battery: ",    get_battery_percentage,  false , 0}, */\
	SPACER \
	{ "Packages: ",   get_packages_pacman,     false , 0}, \
	{ "Explicit: ",   run_shell_cmd,           true  , "pacman -Qe | wc -l"}, \
/* Number of running processes */ \
	{ "Date: ",       get_date,                false , 0}, \
	{ "Uptime: ",     get_uptime,              false , 0}, \
/* Local IP address */ \
	SPACER \
	{ "CPU: ",        get_cpu,                 true  , 0}, \
/* GPU don't work */ \
	{ "GPU: ",        get_gpu,                 true  , 0}, \
	{ "Memory: ",     get_memory,              true  , 0}, \
	{ "Resolution: ", get_resolution,          false , 0}, \
	{ "Disk(/): ",    get_disk_usage,          false , "/"}, \
	{ "Disk(/home): ",get_disk_usage,          false , "/home"}, \
	SPACER \
/* Users */ \
	{ "GTK Theme: ",  get_gtk_option,          false , "gtk-theme-name"}, \
	{ "GTK Icons: ",  get_gtk_option,          false , "gtk-icon-theme-name"}, \
	{ "GTK Font: ",   get_gtk_option,          false , "gtk-font-name"}, \
	{ "Keys: ",       run_shell_cmd,           true  , "setxkbmap -query | grep layout | cut -b 13- | sed 's/,/ /g'"}, \
	{ "",             get_bar,                 false , (void*)25}, \
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
	REMOVE("Integrated Graphics Controller"), \
}

/* GOAL:
print_info() {
	info title
	info underline
	info cols

G	info "OS" distro
G	info "Kernel" kernel
G	info "WM" wm
G	info "Terminal" term
G	info "Shell" shell
	info underline

	info "Users" users
	info "Local IP" local_ip
G	info "Uptime" uptime
G	prin "Date" "$(date)"
G	info "Packages" packages
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
