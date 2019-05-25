
INC=-I/usr/include/cjson/ -I../main -I../components/yy_color_convertor/include
LIB=-lcjson -lm
DEFINES=-DTESTING

test: testWeather.c ../main/parseWeather.c ../components/yy_color_convertor/yy_color_converter.c
	gcc $(INC) $(LIB) $(DEFINES) -o $@ $^
	./$@

clean:
	rm test
