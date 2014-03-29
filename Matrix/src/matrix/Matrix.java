package matrix;

import matrix.operations.MatrixOperations;
import java.io.*;

/**
 * Created by Дмитрий on 13.03.14.
 */
public class Matrix implements FileOperations, ConsolOperations, MatrixBasicOperations {
    protected double[][] data;

    @Override
    public void printToConsole(){
        System.out.println(this.toString());
    }

    @Override
    public void printToFile(String fileName){
        File file = new File(fileName);
        try {
            if(!file.exists()){
                file.createNewFile();
            }
            PrintWriter out = new PrintWriter(file.getAbsoluteFile());
            try {
                out.print(this.toString());
            } finally {
                out.close();
            }
        } catch(IOException e) {
            throw new RuntimeException(e);
        }

    }

    public void setElem(int rowNum, int colNum,double elem){
        this.data[rowNum][colNum] = elem;
    }

    public void setSize(int rows, int columns){
        data = new double[rows][columns];
    }

    public double getElem(int rowNum, int colNum){
        return data[rowNum][colNum];
    }

    @Override
    public int getRows() {
        return this.data.length;
    }

    @Override
    public int getCols() {
        return this.data[0].length;
    }

    @Override
    public double getDet(){
        return MatrixOperations.gauss(this);
    }

    @Override
    public String toString(){
        StringBuilder sb = new StringBuilder();
        for(int i=0; i<data.length; i++){
            for(int j=0; j<data[i].length; j++){
                sb.append(data[i][j] + "   ");
            }
            sb.append("\n");
        }

        return sb.toString();
    }

    @Override
    public double getTrace(){
        double tmp = 0;
            for(int i=0; i<data.length; i++){
                    tmp += data[i][i];
            }
        return tmp;
    }
}

