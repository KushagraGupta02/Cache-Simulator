#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

// long long int updateLRU();     // returns -1 if not occu
void updateLRU(long long int pos, std::vector<long long int>* LRURow) {
    for (long long int i = 0; i < LRURow->size(); i++) {
        if (pos == i) {
            (*LRURow)[i] = 0;
        }
        else {
            (*LRURow)[i]++;
        }
    }
    return;
}
long long int checkLRU(std::vector<long long int> LRURow){
    long long int least=-1;
    long long int pos=-1;
    for(long long int i = 0; i < LRURow.size(); i++){
        if (least<=LRURow[i]){
            least=LRURow[i];
            pos=i;
        }
    }
    return pos;
}


long long int findtag(long long int tag, std::vector<long long int> tagRow) {
    for (long long int i = 0; i < tagRow.size(); i++) {if (tagRow[i] == tag) {return i;}}
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::ifstream infile(argv[6]); // Open the input file
    if (!infile) {
        std::cerr << "Failed to open file: " << argv[1] << std::endl;
        return 1;
    }

    std::string line;
    long long int blocksize = std::stoll(argv[1]);
    long long int l1_size = std::stoll(argv[2]);
    long long int l1_assoc = std::stoll(argv[3]);
    long long int l2_size = std::stoll(argv[4]);
    long long int l2_assoc = std::stoll(argv[5]);
    long long int l1_rows = l1_size/(l1_assoc*blocksize);
    long long int l2_rows = l2_size/(l2_assoc*blocksize);

    std::vector<std::vector<long long int>> L1(l1_rows, std::vector<long long int>(l1_assoc, -1));
    std::vector<std::vector<long long int>> L2(l2_rows, std::vector<long long int>(l2_assoc, -1));

    std::vector<std::vector<long long int>> tagTable1(l1_rows, std::vector<long long int>(l1_assoc,-1));
    std::vector<std::vector<long long int>> tagTable2(l2_rows, std::vector<long long int>(l2_assoc,-1));

    std::vector<std::vector<long long int>> LRU_L1(l1_rows, std::vector<long long int>(l1_assoc,-1));   // to know which block in row is used least recently
    for (long long int i = 0; i < LRU_L1.size();i++){
    for (long long int k = 0; k < LRU_L1[i].size(); k++) {
        LRU_L1[i][k]=LRU_L1[i].size()-k;
        // std::cout<<"LRU L1 " <<LRU_L1[i][k]<< " ";
        
    }
    // std::cout << "| "; 
            // std::cout << std::endl;  // start a new row
    }
    std::vector<std::vector<long long int>> LRU_L2(l2_rows, std::vector<long long int>(l2_assoc,-1));
    for (long long int i = 0; i < LRU_L2.size();i++){
    for (long long int k = 0; k < LRU_L2[i].size(); k++) {
        LRU_L2[i][k]=LRU_L2[i].size()-k;
        // std::cout<<"LRU L2 " <<LRU_L2[i][k]<< " ";
    }
    // std::cout << "| "; 
    //         std::cout << std::endl;  // start a new row
    }
    std::vector<std::vector<long long int>> dirty_L1(l1_rows, std::vector<long long int>(l1_assoc,0)); 
    std::vector<std::vector<long long int>> dirty_L2(l2_rows, std::vector<long long int>(l2_assoc,0)); 

    // [
    //     [0,1,5,6,4,3,7,2]
    //     [7,5,4,2,1,5,6,3]
    // ]

    long long int L1_reads=0, L1_read_misses=0, L1_writes=0, L1_write_misses=0, L1_miss_rate=0, writebacks_from_L1 = 0;
    long long int L2_reads=0, L2_read_misses=0, L2_writes=0, L2_write_misses=0, L2_miss_rate=0, writebacks_from_L2 = 0;

    while (std::getline(infile, line)) { // Read each line of the file
        // std :: cout << line << std::endl;
        // std::cout << blocksize << l1_size << l1_assoc << l2_size << l2_assoc << std::endl; 
        std::istringstream iss(line);
        std::string word1, word2;
        iss >> word1 >> word2;

        long long int address = std::stoll(word2, nullptr, 16);
        long long int tag1 = address/(l1_rows*blocksize);
        long long int index1 = (address/blocksize)%(l1_rows);
        long long int offset = address%blocksize;  // no use as such

        long long int tag2 = address/(l2_rows*blocksize);
        long long int index2 = (address/blocksize)%(l2_rows);

        // std::cout << "tag : " << tag1 << " index1 " << index1 << " " << offset << std::endl;

        if (word1 == "r") {
            if (findtag(tag1, tagTable1[index1])!=-1) {
                updateLRU(findtag(tag1, tagTable1[index1]), &LRU_L1[index1]);
                L1_reads++;
            }
            else {
                if (findtag(tag2, tagTable2[index2])!=-1) {
                    L2_reads++;
                    L1_reads++;
                    L1_read_misses++;
                    updateLRU(findtag(tag2, tagTable2[index2]), &LRU_L2[index2]);
                    long long int changeBlock1 = checkLRU(LRU_L1[index1]);
                    long long int evicted = L1[index1][changeBlock1];
                    if (evicted != -1){
                    //we dont have space to insert in L1, so we evict
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;

                        if (dirty_L1[index1][changeBlock1]==1){
                            long long int tag_evicted = evicted/(l2_rows*blocksize);
                            long long int index_evicted = (evicted/blocksize)%(l2_rows);
                            if (findtag(tag_evicted,tagTable2[index_evicted])!=-1) {
                                // found in L2
                                dirty_L2[index_evicted][findtag(tag_evicted,tagTable2[index_evicted])]=1;
                                dirty_L1[index1][changeBlock1]=0;
                                writebacks_from_L1++;
                                L2_writes++;
                            }
                            else{
                                // bring from L2 to memory
                                L2_write_misses++;
                                L2_writes++;
                                long long int changeBlock2 = checkLRU(LRU_L2[index2]);
                                long long int evicted2 = L2[index2][changeBlock2];
                                if (evicted2 != -1){
                                    // L2 me lao -- evict
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                    if (dirty_L2[index2][changeBlock2]==1){
                                        writebacks_from_L2++;
                                        dirty_L2[index2][changeBlock2]=0;
                                    }
                                    else{
                                    }
                                }
                                else {
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                }
                            }
                        }
                        else{
                            //L1 doesnt have dirty, do nothing
                        }
                    }
                    else{
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;
                    }
                    
                }
                else {
                    // L2, L1 cache miss
                    // update L1, L2, LRU, TagTable, etc
                    // L1[index][];
                    // L1, LRU L1, TagTable1 needs to be changed
                    // L2, LRU L2, TagTable2 needs to be changed

                    L1_reads++;
                    L1_read_misses++;
                    L2_reads++;
                    L2_read_misses++;
                    
                    long long int changeBlock2 = checkLRU(LRU_L2[index2]);
                    long long int evicted2 = L2[index2][changeBlock2];
                    if (evicted2 != -1){
                        // L2 me lao -- evict
                        updateLRU(changeBlock2, &LRU_L2[index2]);
                        L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                        tagTable2[index2][changeBlock2] = tag2;

                        if (dirty_L2[index2][changeBlock2]==1){
                            writebacks_from_L2++;
                            dirty_L2[index2][changeBlock2]=0;
                        }
                        else{
                            // do nothing :) 
                        }
                    }
                    else{
                        updateLRU(changeBlock2, &LRU_L2[index2]);
                        L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                        tagTable2[index2][changeBlock2] = tag2;
                    }

                    long long int changeBlock1 = checkLRU(LRU_L1[index1]);
                    long long int evicted = L1[index1][changeBlock1];
                    if (evicted != -1){
                    //we dont have space to insert in L1, so we evict
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;

                        if (dirty_L1[index1][changeBlock1]==1){
                            long long int tag_evicted = evicted/(l2_rows*blocksize);
                            long long int index_evicted = (evicted/blocksize)%(l2_rows);
                            if (findtag(tag_evicted,tagTable2[index_evicted])!=-1) {
                                // found in L2
                                dirty_L2[index_evicted][findtag(tag_evicted,tagTable2[index_evicted])]=1;
                                dirty_L1[index1][changeBlock1]=0;
                                writebacks_from_L1++;
                                L2_writes++;
                            }
                            else{
                                // bring from L2 to memory
                                L2_write_misses++;
                                L2_writes++;
                                long long int changeBlock2 = checkLRU(LRU_L2[index2]);
                                long long int evicted2 = L2[index2][changeBlock2];
                                if (evicted2 != -1){
                                    // L2 me lao -- evict
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                    if (dirty_L2[index2][changeBlock2]==1){
                                        writebacks_from_L2++;
                                        dirty_L2[index2][changeBlock2]=0;
                                    }
                                    else{
                                        // do nothing :) 
                                        // std::cout<<"oh dear boy3" << std::endl;
                                    }
                                }
                                else {
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                }
                            }
                        }
                        else{
                            //L1 doesnt have dirty, do nothing
                        }
                    }
                    else{
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;
                    }
                }
            }
        }
        else { // word1 is "w"
            if (findtag(tag1, tagTable1[index1])!=-1) {
                updateLRU(findtag(tag1, tagTable1[index1]), &LRU_L1[index1]);
                 dirty_L1[index1][findtag(tag1, tagTable1[index1])] = 1;
                L1_writes ++;
            }
            else {
                if (findtag(tag2, tagTable2[index2])!=-1) {
                    // L2, L1 cache miss
                    // update L1, L2, LRU, TagTable, etc
                    // L1[index][];
                    // L1, LRU L1, TagTable1 needs to be changed
                    // L2, LRU L2, TagTable2 needs to be changed
                    
                    L1_write_misses++;
                    L2_reads++;
                    updateLRU(findtag(tag2, tagTable2[index2]), &LRU_L2[index2]);
                    // long long int changeBlock2 = checkLRU(LRU_L2[index2]);
                    // long long int evicted2 = L2[index2][changeBlock2];
                    // if (evicted2 != -1){
                    //     // L2 me lao -- evict
                    //     updateLRU(changeBlock2, &LRU_L2[index2]);
                    //     L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                    //     tagTable2[index2][changeBlock2] = tag2;

                    //     if (dirty_L2[index2][changeBlock2]==1){
                    //         writebacks_from_L2++;
                    //     }
                    //     else{
                    //         // do nothing :) 
                    //     }
                    // }
                    // else{
                    //             updateLRU(changeBlock2, &LRU_L2[index2]);
                    //             L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                    //             tagTable2[index2][changeBlock2] = tag2;
                    // }

                    long long int changeBlock1 = checkLRU(LRU_L1[index1]);
                    long long int evicted = L1[index1][changeBlock1];
                    if (evicted != -1){
                    //we dont have space to insert in L1, so we evict
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;

                        if (dirty_L1[index1][changeBlock1]==1){
                            long long int tag_evicted = evicted/(l2_rows*blocksize);
                            long long int index_evicted = (evicted/blocksize)%(l2_rows);
                            if (findtag(tag_evicted,tagTable2[index_evicted])!=-1) {
                                // found in L2
                                dirty_L2[index_evicted][findtag(tag_evicted,tagTable2[index_evicted])]=1;
                                dirty_L1[index1][changeBlock1]=0;
                                writebacks_from_L1++;
                                L2_writes++;
                            }
                            else{
                                // bring from L2 to memory
                                L2_write_misses++;
                                L2_writes++;
                                long long int changeBlock2 = checkLRU(LRU_L2[index2]);
                                long long int evicted2 = L2[index2][changeBlock2];
                                if (evicted2 != -1){
                                    // L2 me lao -- evict
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                    if (dirty_L2[index2][changeBlock2]==1){
                                        writebacks_from_L2++;
                                        dirty_L2[index2][changeBlock2]=0;
                                    }
                                    else{
                                        // do nothing :) 
                                        // std::cout<<"oh dear boy3" << std::endl;
                                    }
                                }
                                else {
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                }
                            }
                        }
                        else{
                            //L1 doesnt have dirty, do nothing
                        }
                    }
                    else{
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;
                    }
                    // long long int whichBlock = checkLRU(LRU_L1[index1]);
                    // updateLRU(whichBlock, &LRU_L1[index1]);
                    // dirty_L1[index1][whichBlock] = 1;
                    updateLRU(findtag(tag1, tagTable1[index1]), &LRU_L1[index1]);
                    dirty_L1[index1][findtag(tag1, tagTable1[index1])] = 1;
                    L1_writes++;

                }
                else {
                    // L2, L1 cache miss
                    // update L1, L2, LRU, TagTable, etc
                    // L1[index][];
                    // L1, LRU L1, TagTable1 needs to be changed
                    // L2, LRU L2, TagTable2 needs to be changed
                    L2_reads++;
                    L1_write_misses++;
                    L1_writes++;
                    L2_read_misses++;
                    long long int changeBlock2 = checkLRU(LRU_L2[index2]);
                    long long int evicted2 = L2[index2][changeBlock2];
                    if (evicted2 != -1){
                        // L2 me lao -- evict
                        updateLRU(changeBlock2, &LRU_L2[index2]);
                        L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                        tagTable2[index2][changeBlock2] = tag2;

                        if (dirty_L2[index2][changeBlock2]==1){
                            writebacks_from_L2++;
                            dirty_L2[index2][changeBlock2]=0;
                        }
                        else{
                            // do nothing :) 
                            // std::cout<<"oh dear boy4" << std::endl;
                        }
                    }
                    else{
                                updateLRU(changeBlock2, &LRU_L2[index2]);
                                L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                tagTable2[index2][changeBlock2] = tag2;
                    }
                    // updateLRU(findtag(tag2, tagTable2[index2]), &LRU_L2[index2]);
                    // dirty_L2[index2][findtag(tag2, tagTable2[index2])] = 0;
                    long long int changeBlock1 = checkLRU(LRU_L1[index1]);
                    long long int evicted = L1[index1][changeBlock1];
                    if (evicted != -1){
                    //we dont have space to insert in L1, so we evict
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;

                        if (dirty_L1[index1][changeBlock1]==1){
                            long long int tag_evicted = evicted/(l2_rows*blocksize);
                            long long int index_evicted = (evicted/blocksize)%(l2_rows);
                            if (findtag(tag_evicted,tagTable2[index_evicted])!=-1) {
                                // found in L2
                                dirty_L2[index_evicted][findtag(tag_evicted,tagTable2[index_evicted])]=1;
                                dirty_L1[index1][changeBlock1]=0;
                                writebacks_from_L1++;
                                L2_writes++;
                            }
                            else{
                                // bring from L2 to memory
                                L2_write_misses++;
                                L2_writes++;
                                long long int changeBlock2 = checkLRU(LRU_L2[index2]);
                                long long int evicted2 = L2[index2][changeBlock2];
                                if (evicted2 != -1){
                                    // L2 me lao -- evict
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                    if (dirty_L2[index2][changeBlock2]==1){
                                        writebacks_from_L2++;
                                        dirty_L2[index2][changeBlock2]=0;
                                    }
                                    else{
                                        // do nothing :) 
                                        // std::cout<<"oh dear boy5" << std::endl;
                                    }
                                }
                                else {
                                    updateLRU(changeBlock2, &LRU_L2[index2]);
                                    L2[index2][changeBlock2] = (address/blocksize)*blocksize;
                                    tagTable2[index2][changeBlock2] = tag2;
                                }
                            }
                        }
                        else{
                            //L1 doesnt have dirty, do nothing
                        }
                    }
                    else{
                        updateLRU(changeBlock1, &LRU_L1[index1]);
                        L1[index1][changeBlock1] = (address/blocksize)*blocksize;
                        tagTable1[index1][changeBlock1] = tag1;
                    }
                    // long long int whichBlock = checkLRU(LRU_L1[index1]);
                    // updateLRU(whichBlock, &LRU_L1[index1]);
                    // dirty_L1[index1][whichBlock] = 1;
                    updateLRU(findtag(tag1, tagTable1[index1]), &LRU_L1[index1]);
                    dirty_L1[index1][findtag(tag1, tagTable1[index1])] = 1;
                }
            }

        }

        // for (long long int i = 0; i < L1.size(); i++) {
        //     for (long long int j = 0; j < L1[i].size(); j++) {
        //         std::cout << L1[i][j] << " ";
        //         // separate columns with a vertical bar
        //     }
        //     std::cout << "| "; 
        //     std::cout << std::endl;  // start a new row
        // }

        // for (long long int i = 0; i < L2.size(); i++) {
        //     for (long long int j = 0; j < L2[i].size(); j++) {
        //         std::cout << L2[i][j] << " ";
        //         // separate columns with a vertical bar
        //     }
        //     std::cout << "| "; 
        //     std::cout << std::endl;  // start a new row
        // }
        
    }
    // for (long long int i = 0; i < LRU_L1.size();i++){
    // for (long long int k = 0; k < LRU_L1[i].size(); k++) {
    //     std::cout<<"LRU L1 " <<LRU_L1[i][k]<< " ";
        
    // }
    // std::cout << "| "; 
    //         std::cout << std::endl;  // start a new row
    // }
    // for (long long int i = 0; i < LRU_L2.size();i++){
    // for (long long int k = 0; k < LRU_L2[i].size(); k++) {
    //     std::cout<<"LRU L2 " <<LRU_L2[i][k]<< " ";
    // }
    // std::cout << "| "; 
    //         std::cout << std::endl;  // start a new row
    // }
    std::cout << "===== Simulation Results =====" << std::endl;
    std::cout<<"i. number of L1 reads:\t\t\t\t"<<L1_reads<<"\n";
    std::cout<<"ii. number of L1 read misses:\t\t\t"<<L1_read_misses<<"\n";
    std::cout<<"iii. number of L1 writes:\t\t\t"<<L1_writes<<"\n";
    std::cout<<"iv. number of L1 write misses:\t\t\t"<<L1_write_misses<<"\n";
    std::cout<<"v. L1 miss rate:\t\t\t\t"<< std::fixed << std::setprecision(4)<<((float)L1_read_misses+(float)L1_write_misses)/(L1_reads+L1_writes)<<"\n";
    std::cout<<"vi. number of writebacks from L1 memory:\t "<< std::dec<<writebacks_from_L1<<"\n";
        if (l2_size != 0){
    std::cout<<"vii. number of L2 reads:\t\t\t"<<L2_reads<<"\n";
    std::cout<<"viii. number of L2 read misses:\t\t\t"<<L2_read_misses<<"\n";
    std::cout<<"ix. number of L2 writes:\t\t\t"<<L2_writes<<"\n";
    std::cout<<"x. number of L2 write misses:\t\t\t"<<L2_write_misses<<"\n";
    std::cout<<"xi. L2 miss rate:\t\t\t\t"<<std::fixed << std::setprecision(4)<<((float)L2_read_misses+(float)L2_write_misses)/(L2_reads+L2_writes)<<"\n";
    std::cout<<"xii. number of writebacks from L2 memory:\t "<<writebacks_from_L2<<"\n";
        }
    std::cout<<"Total time taken: "<< (float(L1_reads)+float(L1_writes))*0.000001+(float(L2_reads)+float(L2_writes))*0.00002+(float(L2_read_misses)+float(L2_write_misses)+float(writebacks_from_L2))*0.0002 <<"ms"<<"\n";
    infile.close(); // Close the input file
    return 0;
}

//  <kbd>   cout << "\n\n===== Simulation Results =====";</kbd>
// <kbd>    </kbd>

// <kbd>    cout << "\ni. number of L1 reads:\t\t\t\t" << dec << L1.READ;</kbd>
// <kbd>    cout << "\nii. number of L1 read misses:\t\t\t" << dec << L1.READ_MISS;</kbd>
// <kbd>    cout << "\niii. number of L1 writes:\t\t\t" << dec << L1.WRITE;</kbd>
// <kbd>    cout << "\niv. number of L1 write misses:\t\t\t" << dec << L1.WRITE_MISS;</kbd>
// <kbd>    cout << "\nv. L1 miss rate:\t\t\t\t" << fixed << setprecision(4) << L1.MISS_RATE;</kbd>
// <kbd>    cout << "\nvi. number of writebacks from L1 memory:\t" << dec << L1.WRITE_BACKS;</kbd>
// <kbd>    </kbd>
// <kbd>    if (L2_SIZE != 0)</kbd>
// <kbd>    {</kbd>
// <kbd>        cout << "\nvii. number of L2 reads:\t\t\t" << dec << L2.READ;</kbd>
// <kbd>        cout << "\nviii. number of L2 read misses:\t\t\t" << dec << L2.READ_MISS;</kbd>
// <kbd>        cout << "\nix. number of L2 writes:\t\t\t" << dec << L2.WRITE;</kbd>
// <kbd>        cout << "\nx. number of L2 write misses:\t\t\t" << dec << L2.WRITE_MISS;</kbd>
// <kbd>        cout << "\nxi. L2 miss rate:\t\t\t\t" << fixed << setprecision(4) << ((float)L2.READ_MISS+(float)L2.WRITE_MISS)/ (L2.READ+L2.WRITE);</kbd>
// <kbd>        cout << "\nxii. number of writebacks from L2 memory:\t" << dec << L2.WRITE_BACKS << "\n";</kbd>

// <kbd>    }</kbd>