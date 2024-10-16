// DO NOT COMPILE ME!!!

int main() {
	fmt::print("Hello");
#ifndef NDEBUG
	fmt::print("Exception coming from {}::{}\n", __FILE__, __LINE__+2);
#endif
	throw RunTimeError("Could not do the thing!");
	return 1;
}
