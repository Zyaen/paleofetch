/* Forward-declare our functions so users can mention them in their
 * configs at the top of the file rather than near the bottom. */

static char *get_title(void*),
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
            *get_gpu1(void*),
            *get_gpu2(void*),
            *get_memory(void*),
            *get_disk_usage_root(void*),
            *get_disk_usage_home(void*),
            *get_colors1(void*),
            *get_colors2(void*),
            *spacer(void*);

#define SPACER {"", spacer, false, 0},
#define REMOVE(A) { (A), NULL, sizeof(A) - 1 , 0 }
#define REPLACE(A, B) { (A), (B), sizeof(A) - 1, sizeof(B) - 1 }
