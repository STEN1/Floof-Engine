path ="Assets/Sponza/sponza.mtl"
f = open(path)
content = f.read().replace('\\','/')
f.close()

f = open(path, "w")
f.write(content)
f.close()