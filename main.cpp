#include <iostream>
#include <vector>
#include <stdexcept>

class FATsimulator {
private:
    int pointer_size_in_bits;
    int cluster_size_in_bytes;
    std::vector<int> pointers;
    std::vector<bool> bitfield;

public:
    // Konstruktor
    FATsimulator(int pointer_size_in_bits, int cluster_size_in_bytes)
        : pointer_size_in_bits(pointer_size_in_bits), cluster_size_in_bytes(cluster_size_in_bytes) {
        int num_clusters = 1 << pointer_size_in_bits; // 2^pointer_size_in_bits
        pointers.resize(num_clusters, -1); // Initialisiere Zeiger mit -1 (ungültig)
        bitfield.resize(num_clusters, false); // Initialisiere alle Cluster als frei (false)
    }

    // Setzt den Belegungsstatus eines Clusters
    void setClusterStatus(int index, bool status) {
        if (index < 0 || index >= bitfield.size()) {
            throw std::out_of_range("Index out of range");
        }
        bitfield[index] = status;
    }

    // Gibt den Belegungsstatus eines Clusters zurück
    bool getClusterStatus(int index) const {
        if (index < 0 || index >= bitfield.size()) {
            throw std::out_of_range("Index out of range");
        }
        return bitfield[index];
    }

    // Setzt den Zeiger eines Clusters
    void setPointer(int index, int pointer) {
        if (index < 0 || index >= pointers.size()) {
            throw std::out_of_range("Index out of range");
        }
        pointers[index] = pointer;
    }

    // Gibt den Zeiger eines Clusters zurück
    int getPointer(int index) const {
        if (index < 0 || index >= pointers.size()) {
            throw std::out_of_range("Index out of range");
        }
        return pointers[index];
    }

    // Allocates clusters for a new file and returns the start cluster index
    int allocate(int file_len_in_bytes) {
        if (file_len_in_bytes == 0) return 0;
        
        int num_clusters = (file_len_in_bytes + cluster_size_in_bytes - 1) / cluster_size_in_bytes;
        std::vector<int> allocated_clusters;

        for (int i = 0; i < bitfield.size() && num_clusters > 0; ++i) {
            if (!bitfield[i]) {
                allocated_clusters.push_back(i);
                setClusterStatus(i, true);
                --num_clusters;
            }
        }

        if (num_clusters > 0) {
            for (int cluster : allocated_clusters) {
                setClusterStatus(cluster, false);
            }
            return -1; // Not enough space
        }

        for (size_t i = 0; i < allocated_clusters.size() - 1; ++i) {
            setPointer(allocated_clusters[i], allocated_clusters[i + 1]);
        }
        setPointer(allocated_clusters.back(), -1);

        return allocated_clusters.front();
    }

    // Appends clusters to an existing file and returns the start cluster index
    int append(int file_start_cluster, int append_len_in_bytes) {
        if (file_start_cluster < 0 || !getClusterStatus(file_start_cluster)) {
            return -1; // Invalid start cluster
        }
        if (append_len_in_bytes == 0) return file_start_cluster;

        int num_clusters = (append_len_in_bytes + cluster_size_in_bytes - 1) / cluster_size_in_bytes;
        int current_cluster = file_start_cluster;

        while (getPointer(current_cluster) != -1) {
            current_cluster = getPointer(current_cluster);
        }

        std::vector<int> allocated_clusters;

        for (int i = 0; i < bitfield.size() && num_clusters > 0; ++i) {
            if (!bitfield[i]) {
                allocated_clusters.push_back(i);
                setClusterStatus(i, true);
                --num_clusters;
            }
        }

        if (num_clusters > 0) {
            for (int cluster : allocated_clusters) {
                setClusterStatus(cluster, false);
            }
            return -1; // Not enough space
        }

        for (size_t i = 0; i < allocated_clusters.size() - 1; ++i) {
            setPointer(allocated_clusters[i], allocated_clusters[i + 1]);
        }
        setPointer(current_cluster, allocated_clusters.front());
        setPointer(allocated_clusters.back(), -1);

        return file_start_cluster;
    }

    // Returns a list of cluster indices for a file
    std::vector<int> get_cluster_list(int file_start_cluster) {
        std::vector<int> cluster_list;

        if (file_start_cluster < 0 || !getClusterStatus(file_start_cluster)) {
            return cluster_list; // Return empty list for invalid start cluster
        }

        int current_cluster = file_start_cluster;
        while (current_cluster != -1) {
            cluster_list.push_back(current_cluster);
            current_cluster = getPointer(current_cluster);
        }

        return cluster_list;
    }

    // Returns the index of the cluster containing the byte at the given offset
    int seek_cluster(int file_start_cluster, int start_offset_in_bytes) {
        if (file_start_cluster < 0 || !getClusterStatus(file_start_cluster)) {
            return -1; // Invalid start cluster
        }

        int cluster_index = start_offset_in_bytes / cluster_size_in_bytes;
        int current_cluster = file_start_cluster;

        while (cluster_index > 0 && current_cluster != -1) {
            current_cluster = getPointer(current_cluster);
            --cluster_index;
        }

        return current_cluster;
    }

    // Deletes a file identified by the start cluster
    void delete_file(int file_start_cluster) {
        if (file_start_cluster < 0 || !getClusterStatus(file_start_cluster)) {
            return; // Invalid start cluster
        }

        int current_cluster = file_start_cluster;
        while (current_cluster != -1) {
            int next_cluster = getPointer(current_cluster);
            setClusterStatus(current_cluster, false);
            setPointer(current_cluster, -1);
            current_cluster = next_cluster;
        }
    }

    // Drucke den Status des FATsimulators (zu Debug-Zwecken)
    void printStatus() const {
        std::cout << "Cluster Status:\n";
        for (int i = 0; i < bitfield.size(); ++i) {
            std::cout << "Cluster " << i << ": " << (bitfield[i] ? "Belegt" : "Frei") << ", Zeiger: " << pointers[i] << "\n";
        }

        std::cout<<std::endl;
    }
};

// Beispielverwendung
int main() {
    int cluster_size_small = 1024;
    int cluster_size_big = 2048;

    std::vector<int> file_sizes;

    std::vector<int> file_starts;

    file_sizes.push_back(3);
    file_sizes.push_back(5);
    file_sizes.push_back(7);
    file_sizes.push_back(16);


    FATsimulator sim_s(4, cluster_size_small); 
    FATsimulator sim_l(4, cluster_size_big); 

    std::cout << "s:"<<std::endl;
    sim_s.printStatus();
    std::cout << "l:"<<std::endl;
    sim_l.printStatus();
    std::cout << "\n";

     for(int i = 0; i < file_sizes.size(); i++)
    {
        int file_size = file_sizes[i] * 1024;
        std::cout << "adding file of size: "<< file_size << std::endl;
        file_starts.push_back(sim_s.allocate(file_size));
        sim_s.printStatus();

        std::cout << "adding file of size: "<< file_size << std::endl;
        sim_l.allocate(file_size);
        sim_l.printStatus();
    }   

    std::cout << "removein 1 & 3:"<<std::endl;
    sim_s.delete_file(file_starts[0]);
    sim_s.delete_file(file_starts[2]);

    sim_s.printStatus();

    return 0;
}
