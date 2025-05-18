export interface ProductType {
  id: string;
  name: string;
  description: string;
  price: number;
  image: string;
  category: string;
  featured?: boolean;
}

export interface CategoryType {
  id: string;
  name: string;
  slug: string;
  image: string;
}

export interface OrderType {
  customerName: string;
  email: string;
  phone: string;
  address?: string;
  orderType: 'pickup' | 'delivery';
  pickupDate?: string;
  items: Array<{
    id: string;
    name: string;
    price: number;
    quantity: number;
  }>;
  totalPrice: number;
}