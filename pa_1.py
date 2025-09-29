import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('timings.csv')
df['size_MB'] = df['size_bytes'] / (1024*1024)

# plot size
plt.figure(figsize=(7, 4))
plt.plot(df['size_MB'], df['time_s'], marker='o')
plt.title('File transfer time vs file size')
plt.xlabel('File size (MB)')
plt.ylabel('Time (s)')
plt.grid(True)
plt.tight_layout()
plt.savefig('time_vs_size.png', dpi=150)
plt.show()

# plot throughput
plt.figure(figsize=(7, 4))
plt.plot(df['size_MB'], df['size_MB']/df['time_s'], marker='o')
plt.title('Throughput (MB/s) vs file size')
plt.xlabel('File size (MB)')
plt.ylabel('Throughput (MB/s)')
plt.grid(True)
plt.tight_layout()
plt.savefig('throughput_vs_size.png', dpi=150)
plt.show()
