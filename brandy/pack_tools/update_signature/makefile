

objects = update_signature.o rsa.o hash.o crc.o


edit:$(objects)
	gcc -o signature $(objects) -static
	rm -rf $(objects)


clean:
	rm -rf signature $(objects)