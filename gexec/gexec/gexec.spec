Summary: gexec (cluster remote execution environment)
Name: gexec
Version: 0.3.4
Release: 1
Copyright: GPL
Group: System Environment/Daemons
Source: gexec-0.3.4.tar.gz
Requires: gexec

%description
gexec (cluster remote execution environment)

%prep
%setup

%build
./configure --prefix=/usr --enable-ganglia
make RPM_OPT_FLAGS="$RPM_OPT_FLAGS"

%install
make install
install -D -m 755 config/gexec /etc/xinetd.d/gexec

%clean

%files
%defattr(-, root, root)
/usr/sbin/gexecd
/usr/bin/gexec
/usr/lib/libgexec.a
/usr/include/gexec_lib.h
/etc/xinetd.d/gexec

%post 
echo "Sending SIGUSR2 to xinetd (pid `cat /var/run/xinetd.pid`)"
kill -USR2 `cat /var/run/xinetd.pid`

%postun
echo "Sending SIGUSR2 to xinetd (pid `cat /var/run/xinetd.pid`)"
kill -USR2 `cat /var/run/xinetd.pid`
