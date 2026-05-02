perf stat ./bin/file-cleaner ./generated_files/
perf report

### installed
    sudo dnf5 install -y perf
    sudo dnf5 install -y libasan libubsan
    sudo dnf install procps-ng ( for systemctl ) | echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid
    Best of perf ( echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid )
