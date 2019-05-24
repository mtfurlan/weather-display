
INC=-I/usr/include/cjson/ -I../main -I../components/inaudible_led/include
LIB=-lcjson -lm
DEFINES=-DTESTING

test: testWeather.c ../main/parseWeather.c ../components/inaudible_led/colour.c
	gcc $(INC) $(LIB) $(DEFINES) -o $@ $^
	./$@

clean:
	rm test
