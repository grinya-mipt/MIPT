package matrix;
/**
 * Created by Дмитрий on 27.03.14.
 */
public interface MatrixBasicOperations {
    double getDet();
    double getTrace();
    String toString();

    double getElem(int rowNum, int colNum);
    int getRows();
    int getCols();
    void setElem(int rowNum, int colNum,double elem);
    void setSize(int rows, int columns);

}
