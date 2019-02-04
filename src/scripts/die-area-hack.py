names 	= []
posx 	= []
posy 	= []
caps 	= []

with open('sink_cap.txt') as f:
	for line in f:
		tokens = line.split()
		names.append(tokens[0])
		posx.append(float(tokens[1]))
		posy.append(float(tokens[2]))
		caps.append(float(tokens[3]))

minx = min(posx)
miny = min(posy)
maxx = max(posx)
maxy = max(posy)

#print(" - minx = " + str(minx))
#print(" - miny = " + str(miny))
#print(" - maxx = " + str(maxx))
#print(" - maxy = " + str(maxy))
with open('sink_cap.txt', 'w') as f:
	for i in range(len(posx)):
		f.write(names[i] + " " + str(posx[i]-minx) + " " + str(posy[i]-miny) + " " + str(caps[i]) + "\n")

with open('blks_tmp2.txt') as f1, open('blks.txt', 'w') as f2:
	for line in f1:
		tokens = line.split()
		x1 = float(tokens[0]) - minx
		y1 = float(tokens[1]) - miny
		x2 = float(tokens[2]) - minx
		y2 = float(tokens[3]) - miny
		f2.write(str(x1) + " " + str(y1) + " " + str(x2) + " " + str(y2) + "\n")

with open('die-size.txt', 'w') as f:
	f.write(str(maxx-minx) + " " + str(maxy-miny) + " " + str(minx) + " " + str(miny))

#print("../bin/genHtree -w " + str(maxx-minx) + " -h " + str(maxy-miny) + " -n 256 -s 20 -tech 16")

