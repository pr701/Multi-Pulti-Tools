#define FORWARDN(n,y) "/export:_"#y"=ddraw."#y",@"#n
#define FORWARDO(y) "/export:#"#y"=ddraw.#"#y",@"#y",NONAME"

#pragma comment(linker,FORWARDN(1,DirectDrawCreate))