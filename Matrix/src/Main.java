import matrix.Matrix;
import matrix.operations.MatrixOperations;

import java.io.FileNotFoundException;
import java.util.Random;

/**
 * Created by Дмитрий on 13.03.14.
 */
public class Main {
    public static void main(String args[]) throws FileNotFoundException{
        int n = 3;
        double[][] data = new double[n][n];
        // AP: так не пойдет - задайте универсально пути
        String fileName1 = "D://a.txt";
        String fileName = "D://b.txt";

        Random random = new Random();

        for(int i = 0;i < n;i++)
            for(int j = 0; j < n;j++)
                data[i][j] = random.nextInt(2);

        Matrix mt = new Matrix();
        mt = MatrixOperations.makeMatrix(n, n, data);

        Matrix mt1 = new Matrix();
        mt1 = MatrixOperations.transpose(mt);

        Matrix mt2 = new Matrix();
        mt2 = MatrixOperations.multiplication(mt,mt1);

        mt.printToConsole();
        mt1.printToConsole();
        mt2.printToConsole();
        mt.printToFile(fileName);

        System.out.println(MatrixOperations.readFromFile(fileName1));

        System.out.println(MatrixOperations.gauss(mt));

    }
}
