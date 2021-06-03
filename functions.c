static char *get_title(void *arg) {
	(void)arg;
	// reduce the maximum size for these, so that we don't over-fill the title string
	char hostname[BUF_SIZE / 3];
	status = gethostname(hostname, BUF_SIZE / 3);
	halt_and_catch_fire("unable to retrieve host name");

	char username[BUF_SIZE / 3];
	status = getlogin_r(username, BUF_SIZE / 3);
	halt_and_catch_fire("unable to retrieve login name");

	title_length = strlen(hostname) + strlen(username) + 1;

	char *title = malloc(BUF_SIZE);
	snprintf(title, BUF_SIZE, COLOR "%s\e[0m@" COLOR "%s", username, hostname);

	return title;
}

static char *get_bar(void *arg) {
	intptr_t bar_length = arg;
	if (!bar_length)
		bar_length = title_length;
	char *bar = malloc(BUF_SIZE);
	char *s = bar;
	for (intptr_t i = 0; i < bar_length; i++)
		*(s++) = '-';
	*s = '\0';
	return bar;
}

static char *get_os(void *arg) {
	char *os = malloc(BUF_SIZE), *name = malloc(BUF_SIZE), *line = NULL;
	size_t len;
	FILE *os_release = fopen("/etc/os-release", "r");
	if (os_release == NULL) {
		status = -1;
		halt_and_catch_fire("unable to open /etc/os-release");
	}

	while (getline(&line, &len, os_release) != -1) {
		if (sscanf(line, "NAME=\"%[^\"]+", name) > 0)
			break;
	}

	free(line);
	fclose(os_release);
	snprintf(os, BUF_SIZE, "%s %s", name, uname_info.machine);
	free(name);

	return os;
}

static char *get_kernel(void *arg) {
	char *kernel = malloc(BUF_SIZE);
	strncpy(kernel, uname_info.release, BUF_SIZE);
	return kernel;
}

static char *get_host(void *arg) {
	char *host = malloc(BUF_SIZE), buffer[BUF_SIZE / 2];
	FILE *product_name, *product_version, *model;

	if ((product_name = fopen("/sys/devices/virtual/dmi/id/product_name", "r")) != NULL) {
		if ((product_version = fopen("/sys/devices/virtual/dmi/id/product_version", "r")) != NULL) {
			fread(host, 1, BUF_SIZE / 2, product_name);
			remove_newline(host);
			strcat(host, " ");
			fread(buffer, 1, BUF_SIZE / 2, product_version);
			remove_newline(buffer);
			strcat(host, buffer);
			fclose(product_version);
		} else {
#ifndef HOST_SIMPLE
			fclose(product_name);
			goto model_fallback;
#endif
		}
		fclose(product_name);
		return host;
	}

model_fallback:
	if ((model = fopen("/sys/firmware/devicetree/base/model", "r")) != NULL) {
		fread(host, 1, BUF_SIZE, model);
		remove_newline(host);
		return host;
	}

	status = -1;
	halt_and_catch_fire("unable to get host");
	return NULL;
}

static char *get_uptime(void *arg) {
	long seconds = my_sysinfo.uptime;
	struct {
		char *name;
		int secs;
	} units[] = {
	    {"day", 60 * 60 * 24},
	    {"hour", 60 * 60},
	    {"min", 60},
	};

	int n, len = 0;
	char *uptime = malloc(BUF_SIZE);
	for (int i = 0; i < 3; ++i) {
		if ((n = seconds / units[i].secs) || i == 2) /* always print minutes */
			len += snprintf(uptime + len, BUF_SIZE - len, "%d %s%s, ", n, units[i].name, n != 1 ? "s" : "");
		seconds %= units[i].secs;
	}

	// null-terminate at the trailing comma
	uptime[len - 2] = '\0';
	return uptime;
}

// returns "<Battery Percentage>% [<Charging | Discharging | Unknown>]"
// Credit: allisio - https://gist.github.com/allisio/1e850b93c81150124c2634716fbc4815
static char *get_battery_percentage(void *arg) {
	int battery_capacity;
	FILE *capacity_file, *status_file;
	char battery_status[12] = "Unknown";

	if ((capacity_file = fopen(BATTERY_DIRECTORY "/capacity", "r")) == NULL) {
		status = ENOENT;
		halt_and_catch_fire("Unable to get battery information");
	}

	fscanf(capacity_file, "%d", &battery_capacity);
	fclose(capacity_file);

	if ((status_file = fopen(BATTERY_DIRECTORY "/status", "r")) != NULL) {
		fscanf(status_file, "%s", battery_status);
		fclose(status_file);
	}

	// max length of resulting string is 19
	// one byte for padding incase there is a newline
	// 100% [Discharging]
	// 1234567890123456789
	char *battery = malloc(20);

	snprintf(battery, 20, "%d%% [%s]", battery_capacity, battery_status);

	return battery;
}

static char *get_packages(const char *dirname, const char *pacname, int num_extraneous) {
	int num_packages = 0;
	DIR *dirp;
	struct dirent *entry;

	dirp = opendir(dirname);

	if (dirp == NULL) {
		status = -1;
		halt_and_catch_fire("You may not have %s installed", dirname);
	}

	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_type == DT_DIR)
			num_packages++;
	}
	num_packages -= (2 + num_extraneous); // accounting for . and ..

	status = closedir(dirp);

	char *packages = malloc(BUF_SIZE);
	snprintf(packages, BUF_SIZE, "%d (%s)", num_packages, pacname);

	return packages;
}

static char *get_packages_pacman(void *arg) { return get_packages("/var/lib/pacman/local", "pacman", 0); }

static char *get_shell(void *arg) {
	char *shell = malloc(BUF_SIZE);
	char *shell_path = getenv("SHELL");
	char *shell_name = strrchr(getenv("SHELL"), '/');

	if (shell_name == NULL)                 /* if $SHELL doesn't have a '/' */
		strncpy(shell, shell_path, BUF_SIZE); /* copy the whole thing over */
	else
		strncpy(shell, shell_name + 1, BUF_SIZE); /* o/w copy past the last '/' */

	return shell;
}

static char *get_resolution(void *arg) {
	int screen, width, height;
	char *resolution = malloc(BUF_SIZE);

	if (display != NULL) {
		screen = DefaultScreen(display);

		width = DisplayWidth(display, screen);
		height = DisplayHeight(display, screen);

		snprintf(resolution, BUF_SIZE, "%dx%d", width, height);
	} else {
		DIR *dir;
		struct dirent *entry;
		char dir_name[] = "/sys/class/drm";
		char modes_file_name[BUF_SIZE * 2];
		FILE *modes;
		char *line = NULL;
		size_t len;

		/* preload resolution with empty string, in case we cant find a resolution through parsing */
		strncpy(resolution, "", BUF_SIZE);

		dir = opendir(dir_name);
		if (dir == NULL) {
			status = -1;
			halt_and_catch_fire("Could not open /sys/class/drm to determine resolution in tty mode.");
		}
		/* parse through all directories and look for a non empty modes file */
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_type == DT_LNK) {
				snprintf(modes_file_name, BUF_SIZE * 2, "%s/%s/modes", dir_name, entry->d_name);

				modes = fopen(modes_file_name, "r");
				if (modes != NULL) {
					if (getline(&line, &len, modes) != -1) {
						strncpy(resolution, line, BUF_SIZE);
						remove_newline(resolution);

						free(line);
						fclose(modes);

						break;
					}

					fclose(modes);
				}
			}
		}

		closedir(dir);
	}

	return resolution;
}

static char *get_terminal(void *arg) {
	unsigned char *prop;
	char *terminal = malloc(BUF_SIZE);

	/* check if xserver is running or if we are running in a straight tty */
	if (display != NULL) {

		unsigned long _, // not unused, but we don't need the results
		    window = RootWindow(display, XDefaultScreen(display));
		Atom a, active = XInternAtom(display, "_NET_ACTIVE_WINDOW", True), class = XInternAtom(display, "WM_CLASS", True);

#define GetProp(property) XGetWindowProperty(display, window, property, 0, 64, 0, 0, &a, (int *)&_, &_, &_, &prop);

		GetProp(active);
		window = (prop[3] << 24) + (prop[2] << 16) + (prop[1] << 8) + prop[0];
		free(prop);
		if (!window)
			goto terminal_fallback;
		GetProp(class);

#undef GetProp

		snprintf(terminal, BUF_SIZE, "%s", prop);
		free(prop);
	} else {
	terminal_fallback:
		strncpy(terminal, getenv("TERM"), BUF_SIZE); /* fallback to old method */
		/* in tty, $TERM is simply returned as "linux"; in this case get actual tty name */
		if (strcmp(terminal, "linux") == 0) {
			strncpy(terminal, ttyname(STDIN_FILENO), BUF_SIZE);
		}
	}

	return terminal;
}

static char *get_cpu(void *arg) {
	FILE *cpuinfo = fopen("/proc/cpuinfo", "r"); /* read from cpu info */
	if (cpuinfo == NULL) {
		status = -1;
		halt_and_catch_fire("Unable to open cpuinfo");
	}

	char *cpu_model = malloc(BUF_SIZE / 2);
	char *line = NULL;
	size_t len; /* unused */
	int num_cores = 0, cpu_freq, prec = 3;
	double freq;
	char freq_unit[] = "GHz";

	/* read the model name into cpu_model, and increment num_cores every time model name is found */
	while (getline(&line, &len, cpuinfo) != -1) {
		num_cores += sscanf(line, "model name	: %[^\n@]", cpu_model);
	}
	free(line);
	fclose(cpuinfo);

	FILE *cpufreq = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
	line = NULL;

	if (cpufreq != NULL) {
		if (getline(&line, &len, cpufreq) != -1) {
			sscanf(line, "%d", &cpu_freq);
			cpu_freq /= 1000; // convert kHz to MHz
		} else {
			fclose(cpufreq);
			free(line);
			goto cpufreq_fallback;
		}
	} else {
	cpufreq_fallback:
		cpufreq = fopen("/proc/cpuinfo", "r"); /* read from cpu info */
		if (cpufreq == NULL) {
			status = -1;
			halt_and_catch_fire("Unable to open cpuinfo");
		}

		while (getline(&line, &len, cpufreq) != -1) {
			if (sscanf(line, "cpu MHz : %lf", &freq) > 0)
				break;
		}

		cpu_freq = (int)freq;
	}

	free(line);
	fclose(cpufreq);

	if (cpu_freq < 1000) {
		freq = (double)cpu_freq;
		freq_unit[0] = 'M'; // make MHz from GHz
		prec = 0;           // show frequency as integer value
	} else {
		freq = cpu_freq / 1000.0; // convert MHz to GHz and cast to double

		while (cpu_freq % 10 == 0) {
			--prec;
			cpu_freq /= 10;
		}

		if (prec == 0)
			prec = 1; // we don't want zero decimal places
	}

	/* remove unneeded information */
	for (int i = 0; i < COUNT(cpu_config); ++i) {
		if (cpu_config[i].repl_str == NULL) {
			remove_substring(cpu_model, cpu_config[i].substring, cpu_config[i].length);
		} else {
			replace_substring(cpu_model, cpu_config[i].substring, cpu_config[i].repl_str, cpu_config[i].length, cpu_config[i].repl_len);
		}
	}

	char *cpu = malloc(BUF_SIZE);
	snprintf(cpu, BUF_SIZE, "%s (%d) @ %.*f%s", cpu_model, num_cores, prec, freq, freq_unit);
	free(cpu_model);

	truncate_spaces(cpu);

	if (num_cores == 0)
		*cpu = '\0';
	return cpu;
}

static char *find_gpu(int index) {
	// inspired by https://github.com/pciutils/pciutils/edit/master/example.c
	/* it seems that pci_lookup_name needs to be given a buffer, but I can't for the life of my figure out what its for */
	char buffer[BUF_SIZE], *device_class, *gpu = malloc(BUF_SIZE);
	struct pci_access *pacc;
	struct pci_dev *dev;
	int gpu_index = 0;
	bool found = false;

	pacc = pci_alloc();
	pci_init(pacc);
	pci_scan_bus(pacc);
	dev = pacc->devices;

	while (dev != NULL) {
		pci_fill_info(dev, PCI_FILL_IDENT);
		device_class = pci_lookup_name(pacc, buffer, sizeof(buffer), PCI_LOOKUP_CLASS, dev->device_class);
		if (strcmp("VGA compatible controller", device_class) == 0 || strcmp("3D controller", device_class) == 0) {
			strncpy(gpu, pci_lookup_name(pacc, buffer, sizeof(buffer), PCI_LOOKUP_DEVICE | PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id), BUF_SIZE);
			if (gpu_index == index) {
				found = true;
				break;
			} else {
				gpu_index++;
			}
		}

		dev = dev->next;
	}

	if (found == false)
		*gpu = '\0'; // empty string, so it will not be printed

	pci_cleanup(pacc);

	/* remove unneeded information */
	for (int i = 0; i < COUNT(gpu_config); ++i) {
		if (gpu_config[i].repl_str == NULL) {
			remove_substring(gpu, gpu_config[i].substring, gpu_config[i].length);
		} else {
			replace_substring(gpu, gpu_config[i].substring, gpu_config[i].repl_str, gpu_config[i].length, gpu_config[i].repl_len);
		}
	}

	truncate_spaces(gpu);

	return gpu;
}

static char *get_gpu1(void *arg) { return find_gpu(0); }

static char *get_gpu2(void *arg) { return find_gpu(1); }

static char *get_memory(void *arg) {
	int total_memory, used_memory;
	int total, shared, memfree, buffers, cached, reclaimable;

	FILE *meminfo = fopen("/proc/meminfo", "r"); /* get infomation from meminfo */
	if (meminfo == NULL) {
		status = -1;
		halt_and_catch_fire("Unable to open meminfo");
	}

	/* We parse through all lines of meminfo and scan for the information we need */
	char *line = NULL; // allocation handled automatically by getline()
	size_t len;        /* unused */

	/* parse until EOF */
	while (getline(&line, &len, meminfo) != -1) {
		/* if sscanf doesn't find a match, pointer is untouched */
		sscanf(line, "MemTotal: %d", &total);
		sscanf(line, "Shmem: %d", &shared);
		sscanf(line, "MemFree: %d", &memfree);
		sscanf(line, "Buffers: %d", &buffers);
		sscanf(line, "Cached: %d", &cached);
		sscanf(line, "SReclaimable: %d", &reclaimable);
	}

	free(line);

	fclose(meminfo);

	/* use same calculation as neofetch */
	used_memory = (total + shared - memfree - buffers - cached - reclaimable) / 1024;
	total_memory = total / 1024;
	int percentage = (int)(100 * (used_memory / (double)total_memory));

	char *memory = malloc(BUF_SIZE);
	snprintf(memory, BUF_SIZE, "%dMiB / %dMiB (%d%%)", used_memory, total_memory, percentage);

	return memory;
}

static char *get_disk_usage(const char *folder) {
	char *disk_usage = malloc(BUF_SIZE);
	long total, used, free;
	int percentage;
	status = statvfs(folder, &file_stats);
	halt_and_catch_fire("Error getting disk usage for %s", folder);
	total = file_stats.f_blocks * file_stats.f_frsize;
	free = file_stats.f_bfree * file_stats.f_frsize;
	used = total - free;
	percentage = (used / (double)total) * 100;
#define TO_GB(A) ((A) / (1024.0 * 1024 * 1024))
	snprintf(disk_usage, BUF_SIZE, "%.1fGiB / %.1fGiB (%d%%)", TO_GB(used), TO_GB(total), percentage);
#undef TO_GB
	return disk_usage;
}

static char *get_disk_usage_root(void *arg) { return get_disk_usage("/"); }

static char *get_disk_usage_home(void *arg) { return get_disk_usage("/home"); }

static char *get_colors1(void *arg) {
	char *colors1 = malloc(BUF_SIZE);
	char *s = colors1;

	for (int i = 0; i < 8; i++) {
		sprintf(s, "\e[4%dm   ", i);
		s += 8;
	}
	snprintf(s, 5, "\e[0m");

	return colors1;
}

static char *get_colors2(void *arg) {
	char *colors2 = malloc(BUF_SIZE);
	char *s = colors2;

	for (int i = 8; i < 16; i++) {
		sprintf(s, "\e[48;5;%dm   ", i);
		s += 12 + (i >= 10 ? 1 : 0);
	}
	snprintf(s, 5, "\e[0m");

	return colors2;
}

static char *spacer(void *arg) {
	return calloc(1, 1); // freeable, null-terminated string of length 1
}
