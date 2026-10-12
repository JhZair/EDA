===========================================================================
 KIT DE PRACTICA  --  R-tree del examen
===========================================================================

QUE ES ESTO
  Tres ejercicios sobre el MISMO codigo del examen (RTree.h / RTree.cpp).
  En cada uno hay una sola funcion vaciada. La escribes, compilas, corres
  el test y el test te dice si quedo bien, sin esperar a nadie.

COMO SE USA
  1. Entra a la carpeta del ejercicio.
  2. Abre RTree.cpp y busca el bloque de comentarios que dice EJERCICIO.
     Ahi esta la funcion vaciada, con los pasos del paper como guia.
  3. Escribela.
  4. Compila y corre:

         g++ -std=c++14 -o test RTree.cpp verificador.cpp test_rtree.cpp
         ./test

     En ZinjaI: Archivo > Nuevo > Proyecto, y agrega los cuatro archivos
     (RTree.h, RTree.cpp, verificador.cpp, test_rtree.cpp).

  5. Si sale ">>> TODO CORRECTO", esta bien. Si no, lee el mensaje: te dice
     que propiedad del paper se rompio, en que nivel y con que valores.

LOS EJERCICIOS
  ej1_InsertRectRec    Insert, pasos I1-I3. Es la que cayo el ano pasado.
                       Empieza por esta.
  ej2_PickSeeds        PickSeeds, pasos PS1-PS2 del split cuadratico.
  ej3_ChoosePartition  Quadratic Split, pasos QS1-QS3. La mas larga; el
                       PickNext del paper va aqui adentro.

QUE COMPRUEBA EL TEST
  - Propiedades (1) a (6) de la seccion 2 del paper, despues de CADA
    insercion y de cada borrado.
  - Que el numero de registros guardados sea el correcto.
  - La "firma" del arbol: un numero que resume su estructura y sus MBR.
    Si tu firma es 129889876, tu arbol quedo identico al que produce la
    implementacion correcta. Esto importa sobre todo en ej2 y ej3: un
    split mal hecho produce un arbol VALIDO pero de peor calidad, y solo
    la firma lo detecta.

ARCHIVOS QUE NO DEBES TOCAR
  verificador.cpp   el que comprueba las propiedades
  test_rtree.cpp    el banco de pruebas
  RTree.h           igual al del examen, mas las tres declaraciones del
                    verificador al final de cada seccion (marcadas)

SOLUCIONES
  En la carpeta soluciones/ hay un .txt por ejercicio. Abrelos SOLO
  despues de haber intentado escribir la funcion. Mirarlas antes te hace
  sentir que entendiste sin haber aprendido a producirlo, que es
  justamente lo que falla en el examen.

NOTA SOBRE EL CODIGO
  RTree.cpp es el del examen con InsertRectRec ya implementada (salvo en
  ej1, donde esa es justamente la tarea). Lo demas esta intacto: mismos
  nombres, mismo estilo con m_ y a_, mismos arreglos fijos.
