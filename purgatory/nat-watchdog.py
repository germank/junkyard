#!/usr/bin/env python2.4
import os, sys, re, time, syslog

PID_FILE = '/var/run/nat-watchdog.pid'
CACHE_FILE = '/var/cache/nat-watchdog.cache'

os.environ['PATH'] = '/usr/local/sbin:/usr/sbin:/sbin:/usr/local/bin:/usr/bin:/bin'

def test_internet_connectivity():
    if os.system('ping -c 1 ya.ru >/dev/null') == 0:
        return True
    if os.system('dig google.com @4.2.2.4 >/dev/null') == 0:
        return True
    return False

def log(message):
    syslog.syslog(syslog.LOG_DAEMON | syslog.LOG_ERR, 'nat-watchdog: ' + message)
    sys.stderr.write(message + '\n')

def main_loop():
    start_time = time.time()
    last_named_and_dhcpd_restart = time.time()

    # read a list of past 'Copper Link Is Down' messages
    handled_suspects = set()
    if os.path.exists(CACHE_FILE):
        for line in file(CACHE_FILE, 'r'):
            handled_suspects.add(line.strip())

    time.sleep(300)  # if this script misbehaves, let sysadmin have a chance to disable it
    log('standing by')

    while True:
        # check /var/log/messages for new 'Copper Link Is Down' messages
        new_suspects = []
        for line in file('/var/log/messages'):
            line = line.strip()
            if 'NIC Copper Link is Down' in line:
                if line not in handled_suspects:
                    new_suspects.append(line)
        if len(new_suspects) != 0:
            log('intranet connectivity seems to be lost')
            os.system('ifdown eth0; sleep 1; ifup eth0')
            os.system('ifdown eth1; sleep 1; ifup eth1')
            for s in new_suspects:
                handled_suspects.add(s)
                file(CACHE_FILE, 'a').write(s + '\n')

        # test that we have internet connectivity, take action if we don't
        num_failures = 0
        while not test_internet_connectivity():
            log('internet connectivity seems to be lost')
            num_failures += 1
            if num_failures <= 3:
                os.system('ifdown eth0; sleep 1; ifup eth0')
                if num_failures <= 2:
                    time.sleep(5)
                else:
                    time.sleep(60)
            else:
                log('rebooting')
                os.system('shutdown -f -r now')

        # periodically reboot
        if time.time() - start_time > 12*3600:
            log('initiating routine reboot')
            os.system('sync; shutdown -f -r now')

        # periodically restart named and dhcpd
        if time.time() - last_named_and_dhcpd_restart > 2*3600:
            os.system('/etc/init.d/dhcpd restart')
            os.system('/etc/init.d/named restart')
            last_named_and_dhcpd_restart = time.time()

        os.system('sync')
        time.sleep(60)


# Does "double fork magic", the recommended unix way of daemonizing
def daemonize():
    try:
        pid = os.fork()
        if pid > 0:
            sys.exit(0)
    except OSError, e:
        sys.stderr.write('fork failed\n')
        sys.exit(3)

    os.chdir('/')
    os.setsid()
    os.umask(0022)

    try:
        pid = os.fork()
        if pid > 0:
            sys.exit(0)
    except OSError, e:
        sys.stderr.write('fork failed\n')
        sys.exit(3)

    file(PID_FILE, 'w').write('%s\n' % os.getpid())


def main():
    if os.getuid() != 0:
        sys.stderr.write('You must run this program under root.\n')
        sys.exit(1)

    for required_binary in ['ping', 'dig', 'ifconfig', 'shutdown']:
        if os.system('which %s >/dev/null 2>&1' % required_binary) != 0:
            sys.stderr.write('Error: could not find program "%s"\n' % required_binary)
            sys.exit(2)

    daemonize()
    main_loop()

if __name__ == '__main__':
    main()
