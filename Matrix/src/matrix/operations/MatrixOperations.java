package matrix.operations;

import matrix.Matrix;
import matrix.size.MatrixSize;

import java.io.*;

/**
 * Created by Дмитрий on 28.03.14.
 */
public class MatrixOperations{
    private MatrixOperations(){
        }

    public static Matrix creatMatrix(int rows, int columns){
        Matrix mt = new Matrix();
        mt.setSize(rows, columns);
        return mt;
    }

    protected static MatrixSize getSize(Matrix mt){
        int rows = mt.getRows();
        int cols = mt.getCols();
        return new MatrixSize(rows, cols);
    }

    public static Matrix makeMatrix(int rows, int columns, double[][] data){
        Matrix mt = creatMatrix(rows, columns);
        for(int i=0; i<data.length; i++){
            for(int j=0; j<data[i].length; j++){
                mt.setElem(i, j, data[i][j]);
            }
        }
        return mt;
    }

    public static Matrix addition(Matrix mt1, Matrix mt2){
        int rows = getSize(mt1).getRows();
        int cols = getSize(mt1).getColumns();
        int rows2 = getSize(mt2).getRows();
        int cols2 = getSize(mt2).getColumns();
        if ((rows == rows2) && (cols == cols2)) {
            Matrix mt = creatMatrix(rows, cols);
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    mt.setElem(i, j, mt1.getElem(i, j) + mt2.getElem(i, j));
                }
            }
            return mt;
        } else {
            return null;
        }
    }

    public static Matrix multiplication(Matrix mt1, Matrix mt2) {
        int rows = getSize(mt1).getRows();
        int cols = getSize(mt1).getColumns();
        int rows2 = getSize(mt2).getRows();
        int cols2 = getSize(mt2).getColumns();
        if (cols == rows2) {
            Matrix mt = creatMatrix(rows, cols2);
            double tmp = 0;
            for (int i = 0; i < rows; i++) {
                for (int c = 0; c < cols2; c++) {
                    for (int j = 0; j < cols; j++) {
                        tmp += mt1.getElem(i, j) * mt2.getElem(j, c);
                    }
                    mt.setElem(i, c, tmp);
                    tmp = 0;
                }
            }
            return mt;
        } else {
            return null;
        }
    }

    public static Matrix swapLine(int spLine, int targetLine, Matrix mt1) {
        int rows = getSize(mt1).getRows();
        int cols = getSize(mt1).getColumns();
        double tmp;
        for (int firstLine=spLine; spLine<rows; spLine++) {
            for (int j=0; j<cols; j++) {
                tmp = mt1.getElem(firstLine, j);
                mt1.setElem(firstLine, j, mt1.getElem(targetLine, j));
                mt1.setElem(targetLine, j, tmp);
            }
        }
        return mt1;
    }

    public static Matrix transpose (Matrix mt1) {
        int rows = getSize(mt1).getRows();
        int cols = getSize(mt1).getColumns();
        Matrix mt = creatMatrix(rows, cols);
        double tmp = 0;
        for (int i=0; i<rows; i++){
            for (int j=0; j<cols; j++){
                tmp = mt1.getElem(i, j);
                mt.setElem(j, i, tmp);
            }
        }
        return mt;
    }

    public static double gauss (Matrix mt1) {
        int rows = getSize(mt1).getRows();
        int cols = getSize(mt1).getColumns();
        double result = 1;
        int count;
    /*копируем полученную матрицу в  mt */
        Matrix mt = new Matrix();
        mt.setSize(rows, cols);
        for (int i=0; i<rows; i++){
            for (int j=0; j<cols; j++){
                mt.setElem(i, j, mt1.getElem(i, j));
            }
        }
    /*копируем полученную матрицу в матрицу mt2, в неё
     копируется "рабочая матрица" mt, после зануления очередного столбца*/
        Matrix mt2 = new Matrix();
        mt2.setSize(rows, cols);
        for (int i=0; i<rows; i++){
            for (int j=0; j<cols; j++){
                mt2.setElem(i, j, mt1.getElem(i, j));
            }
        }

        for (int i=0; i<cols; i++){

            //проверка на НЕнулевой i-тый элемент строки
            if (mt.getElem(i, i)==0 && i<rows-1)
            {
                count = i;
                do{
                    mt.getElem(count, 0);
                    count++;
                }while(mt.getElem(count, 0) == 0);
                mt = MatrixOperations.swapLine(i, count, mt);
                result = -result; // Если была совершена перестановка —
            }                 // меняем знак определителя

        /*копирование "рабочей" матрицы в "статическую", необходимо
        для корректного расчета коофициентов умножения строк перед вычитанием*/
            for (int x=0; x<rows; x++){
                for (int y=0; y<cols; y++){
                    mt2.setElem(x, y, mt.getElem(x, y));
                }
            }

            //зануление i-того столбца
            for (int j=i+1; j<rows; j++){
                for (int k=i; k<cols; k++){
                    double tmp = mt.getElem(j, k)-((mt.getElem(i, k)*mt2.getElem(j, i))/mt.getElem(i, i));
                    mt.setElem(j, k, tmp);
                }
            }

        }
        //Вычисление определителя
        for (int x=0; x<rows; x++){
            result = result*mt.getElem(x, x);
        }

        return result;
    }

    public static String readFromFile(String fileName) throws FileNotFoundException {
        StringBuilder sb = new StringBuilder();

        File file = new File(fileName);
        if (!file.exists()){
            throw new FileNotFoundException(file.getName());
        }
        try {
            BufferedReader in = new BufferedReader(new FileReader(file.getAbsoluteFile()));
            try {
                String s;
                while ((s = in.readLine()) != null) {
                    sb.append(s);
                    sb.append("\n");
                }
            } finally {
                in.close();
            }
        } catch(IOException e) {
            throw new RuntimeException(e);
        }
        return sb.toString();
    }

}
