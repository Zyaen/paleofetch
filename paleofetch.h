/* Forward-declare our functions so users can mention them in their
 * configs at the top of the file rather than near the bottom. */

static char
	*get_title(void*),
	*get_bar(void*),      //If argument is 0, then prints a bar the length of the title. If argument is nonzero, prints a bar argument characters long
	*get_os(void*),
	*get_kernel(void*),
	*get_host(void*),
	*get_uptime(void*),
	*get_battery_percentage(void*),
	*get_packages_pacman(void*),
	*get_shell(void*),
	*get_resolution(void*),
	*get_terminal(void*),
	*get_cpu(void*),
	*get_gpu(void*),     // The argument is the GPU number to select. Original paleofetch had getgpu1(), getgpu2(). These would now be get_gpu(0), get_gpu(1), ...
	*get_memory(void*),
	*get_disk_usage(void*), //The argument is the path under which to search, for example "/" or "/home"
	*get_colors1(void*),
	*get_colors2(void*),
	*spacer(void*),
	
	*run_shell_cmd(void*),  //The argument is a POSIX shell-compliant line of code to be executed
	*get_date(void*),
	*get_gtk_option(void*), //The argument is the gtk option you want to get, such as 'gtk-theme-name'
	*get_env(void*);       //The argument is the name of the environment variable to get

#define SPACER {"", spacer, false, 0},
#define REMOVE(A) { (A), NULL, sizeof(A) - 1 , 0 }
#define REPLACE(A, B) { (A), (B), sizeof(A) - 1, sizeof(B) - 1 }
